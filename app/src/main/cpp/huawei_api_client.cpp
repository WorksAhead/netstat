#include "huawei_api_client.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/date_time.hpp>

#include "huawei_api_message.pb.h"

using namespace huawei::api;
using namespace huawei_api_client;
using boost::asio::ip::tcp;

// interface
typedef void(*huawei_api_client_log_callback_t)(int error_code, const char* description);

void* huawei_api_client_create(const char* address, const char* port);
void huawei_api_client_destory(void* instance);

void huawei_api_client_set_log_callback(void* instance, huawei_api_client_log_callback_t log_callback);
void huawei_api_client_set_log_file(void* instance, const char* filename);

void huawei_api_client_start(void* instance);
void huawei_api_client_stop(void* instance);
// end

// Jiang's
namespace Internals {

    inline void bindArgs(int argN, boost::format&)
    {
    }

    template<typename Arg1, typename... Args>
    inline void bindArgs(int argN, boost::format& f, const Arg1& arg1, Args... args)
    {
        f.bind_arg(argN, arg1);
        bindArgs(argN + 1, f, args...);
    }
}

template<typename... Args>
inline void bindArgs(boost::format& f, Args... args)
{
    Internals::bindArgs(1, f, args...);
}
// end

HuaweiApiClient::HuaweiApiClient(const std::string& address, 
                                 const std::string& port)
    : addr_{address}
    , port_{port}
    , socket_{io_service_} {
    RegisterResponseHandler();
}

HuaweiApiClient::~HuaweiApiClient() {
    io_service_.stop();

    qos_state_ = kStoppedQosService;

    StopQosRequestTimer();
    StopHeartbeat();
    
    if (socket_.is_open()) {
        Close();
    }

    signal_.disconnect_all_slots();
}

boost::signals2::connection HuaweiApiClient::RegisterCallback(
    const SignalType::slot_type& subscriber) {
    return signal_.connect(subscriber);
}

void HuaweiApiClient::Start() {
    if (qos_state_ == kApplyingQosRequest || qos_state_ == kUnderQosService)
    {
        signal_(-1, "<error> Failed to Start HuaweiApiClient. Huawei QoS has already applied.");
        log("<error> Failed to Start HuaweiApiClient. Huawei QoS has already applied.");
        return;
    }

    if (qos_state_ == kRemovingQosRequest)
    {
        StopQosRequestTimer();
    }

    signal_(0, "Start HuaweiApiClient.");
    log("Start HuaweiApiClient.");

    qos_state_ = kApplyingQosRequest;

    if (connection_state_ == kDisconnected)
    {
        try {
            Connect();
        } catch (boost::system::system_error& error) {
            connection_state_ = kDisconnected;
            // qos_state_ = kStoppedQosService;

            signal_(-1, boost::str(boost::format(
              "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());
            log(boost::str(boost::format(
              "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());

            // Start qos request timeout timer.
            qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
            qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));
        }
    }
    else
    {
        DoApplyQosRequest();
    }
}

void HuaweiApiClient::Stop() {
    if (qos_state_ == kStoppedQosService || qos_state_ == kRemovingQosRequest)
    {
        signal_(-1, "<error> Failed to Stop HuaweiApiClient. Huawei QoS has already been removed.");
        log("<error> Failed to Stop HuaweiApiClient. Huawei QoS has already been removed.");
        return;
    }

    if (qos_state_ == kApplyingQosRequest)
    {
        StopQosRequestTimer();
    }

    if (qos_state_ == kUnderQosService)
    {
        StopHeartbeat();
    }
    
    signal_(0, "Stop HuaweiApiClient.");
    log("Stop HuaweiApiClient.");

    qos_state_ = kRemovingQosRequest;

    DoRemoveQosRequest();
}

void HuaweiApiClient::Run() {
    boost::asio::io_service::work work(io_service_);
    io_service_.run();
}

void HuaweiApiClient::RegisterResponseHandler() {
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kApplyQosResponse, boost::bind(&HuaweiApiClient::ApplyQoSResponse, this, _1));
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kRemoveQosResponse, boost::bind(&HuaweiApiClient::RemoveQoSResponse, this, _1));
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kHeartbeatResponse, boost::bind(&HuaweiApiClient::ReplyHeartbeatResponse, this, _1));
}

void HuaweiApiClient::Connect() {
    signal_(0, "Connecting Huawei Api Server...");
    log("Connecting Huawei Api Server...");

    connection_state_ = kConnecting;

    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(addr_, port_);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::async_connect(socket_, endpoint_iterator,
        boost::bind(&HuaweiApiClient::HandleConnect, this,
            boost::asio::placeholders::error));
}

void HuaweiApiClient::Close() {
    connection_state_ = kDisconnected;
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void HuaweiApiClient::DoApplyQosRequest() {
    signal_(0, "Send ApplyQoSRequest to Huawei Api Server.");
    log("Send ApplyQoSRequest to Huawei Api Server.");

    // Start qos request timeout timer.
    qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
    qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));

    ApplyQosRequest* qos_request = new ApplyQosRequest;
    qos_request->set_local_ip(socket_.local_endpoint().address().to_string());

    HuaweiApiMessage api_message;
    api_message.set_allocated_apply_qos_request(qos_request);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&HuaweiApiClient::HandleWrite, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void HuaweiApiClient::DoRemoveQosRequest() {
    signal_(0, "Send RemoveQoSRequest to Huawei Api Server.");
    log("Send RemoveQoSRequest to Huawei Api Server.");

    // Start qos request timeout timer.
    qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
    qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));

    RemoveQosRequest* qos_request = new RemoveQosRequest;

    HuaweiApiMessage api_message;
    api_message.set_allocated_remove_qos_request(qos_request);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&HuaweiApiClient::HandleWrite, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void HuaweiApiClient::StartHeartbeat()
{
    is_heartbeat_pendding = true;

    HeartbeatRequest* heartbeat_request = new HeartbeatRequest;

    HuaweiApiMessage api_message;
    api_message.set_allocated_heartbeat_request(heartbeat_request);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&HuaweiApiClient::HandleWrite, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

    if (heartbeat_timer_ == nullptr)
    {
        heartbeat_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(heartbeat_interval_)));
    }
    else
    {
        heartbeat_timer_->expires_at(heartbeat_timer_->expires_at() + boost::posix_time::seconds(heartbeat_interval_));
    }
    heartbeat_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleHeartbeat, this, boost::asio::placeholders::error));
}

void HuaweiApiClient::StopHeartbeat()
{
    is_heartbeat_pendding = false;

    if (heartbeat_timer_ != nullptr)
    {
        heartbeat_timer_->cancel();
        heartbeat_timer_ = nullptr;
    }
}

void HuaweiApiClient::StartQosRequestTimer()
{
    if (qos_request_timer_ == nullptr)
    {
        qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
    }
    qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));
}

void HuaweiApiClient::StopQosRequestTimer()
{
    if (qos_request_timer_ != nullptr)
    {
        qos_request_timer_->cancel();
        qos_request_timer_ = nullptr;
    }
}

void HuaweiApiClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        signal_(0, "Connected to Huawei Api Server.");
        log("Connected to Huawei Api Server.");

        connection_state_ = kConnected;
        if (qos_state_ == kApplyingQosRequest)
        {
            DoApplyQosRequest();
        }
        else if (qos_state_ == kRemovingQosRequest)
        {
            DoRemoveQosRequest();
        }
    }
    else
    {
        signal_(-1, "<error> Failed to connect to Huawei Api Server.");
        log("<error> Failed to connect to Huawei Api Server.");

        connection_state_ = kDisconnected;

        if (qos_state_ == kApplyingQosRequest)
        {
            // Start qos request timeout timer.
            qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
            qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));

            return;
        }

        //StopQosRequestTimer();
        //StopHeartbeat();
        if (qos_state_ == kRemovingQosRequest)
        {
            signal_(0, "Stop the HuaweiApiClient at local.");
            log("Stop the HuaweiApiClient at local.");

            qos_state_ = kStoppedQosService;
        }
    }
}

void HuaweiApiClient::HandleWrite(const boost::system::error_code& error, std::size_t bytesTransferred)
{
    if (!error)
    {
        // Ready to read response.
        socket_.async_receive(boost::asio::buffer(recv_buff_),
            boost::bind(&HuaweiApiClient::HandleRead, this, 
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        signal_(-1, "<error> Failed to write data to Huawei Api Server, trying to reconnect the server.");
        log("<error> Failed to write data to Huawei Api Server, trying to reconnect the server.");

        // Write data failed, close and reconnect the server.
        Close();

        StopQosRequestTimer();
        StopHeartbeat();

        try {
          Connect();
        }
        catch (boost::system::system_error& error) {
          connection_state_ = kDisconnected;
          // qos_state_ = kStoppedQosService;

          signal_(-1, boost::str(boost::format(
            "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());
          log(boost::str(boost::format(
            "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());

          if (qos_state_ == kUnderQosService)
          {
              qos_state_ = kApplyingQosRequest;
          }

          // Start qos request timeout timer.
          qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
          qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));
        }
    }
}

void HuaweiApiClient::HandleRead(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        HuaweiApiMessage api_message;
        if (!api_message.ParseFromArray(recv_buff_.data(), (int)bytes_transferred))
        {
            signal_(-1, "<error> Failed to parse HuaweiApiMessage.");
            log("<error> Failed to parse HuaweiApiMessage.");
            return;
        }

        HuaweiApiMessage::MessageTypeCase sub_message_type = api_message.message_type_case();
        const google::protobuf::FieldDescriptor* sub_message_descriptor = api_message.GetDescriptor()->FindFieldByNumber(sub_message_type);
        const google::protobuf::Message& sub_message = api_message.GetReflection()->GetMessage(api_message, sub_message_descriptor);

        // Call handler.
        MessageHandler::Handler handler = response_handler_.GetHandler(sub_message_type);
        if (handler != nullptr)
        {
            handler(sub_message);
        }
        else
        {
            signal_(-1, "<error> Receive unknown message, ignore it.");
            log("<error> Receive unknown message, ignore it.");
        }
    }
    else
    {
        Close();

        if (qos_state_ != kStoppedQosService)
        {
            // Read data failed, close and reconnect the server.
            signal_(-1, "<error> Failed to read data from Huawei Api Server, connection has broken, trying to reconnect the server.");
            log("<error> Failed to read data from Huawei Api Server, connection has broken, trying to reconnect the server.");

            StopQosRequestTimer();
            StopHeartbeat();

            try {
              Connect();
            }
            catch (boost::system::system_error& error) {
              connection_state_ = kDisconnected;
              // qos_state_ = kStoppedQosService;

              signal_(-1, boost::str(boost::format(
                "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());
              log(boost::str(boost::format(
                "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());

              if (qos_state_ == kUnderQosService)
              {
                  qos_state_ = kApplyingQosRequest;
              }

              // Start qos request timeout timer.
              qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
              qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));
            }
        }
    }
}

void HuaweiApiClient::HandleQosTimeout(const boost::system::error_code& error) {
    if (error == boost::asio::error::operation_aborted)
    {
        // Timer was not cancelled.
        return;
    }

    if (qos_state_ == kApplyingQosRequest)
    {
        signal_(-1, "<error> ApplyQoSRequest timeout.");
        log("<error> ApplyQoSRequest timeout.");

        if (connection_state_ != kConnected)
        {
            try {
              Connect();
            }
            catch (boost::system::system_error& error) {
              connection_state_ = kDisconnected;
              // qos_state_ = kStoppedQosService;

              //StopQosRequestTimer();
              //StopHeartbeat();

              signal_(-1, boost::str(boost::format(
                "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());
              log(boost::str(boost::format(
                "<error> Failed to connect to HuaweiApiServer. %1%") % error.what()).c_str());

              // Start qos request timeout timer.
              qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
              qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this, boost::asio::placeholders::error));
            }
        }
        else
        {
            DoApplyQosRequest();
        }
    }

    if (qos_state_ == kRemovingQosRequest)
    {
        signal_(-1, "<error> RemoveQoSRequest timeout.");
        log("<error> RemoveQoSRequest timeout.");

        signal_(0, "Stop the HuaweiApiClient at local.");
        log("Stop the HuaweiApiClient at local.");

        qos_state_ = kStoppedQosService;
    }

    // StopQosRequestTimer();
}

void HuaweiApiClient::HandleHeartbeat(const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted)
    {
      // Timer was not cancelled.
      is_heartbeat_pendding = false;
      return;
    }

    // Previous heartbeat did not arrive.
    if (is_heartbeat_pendding)
    {
        signal_(-1, "<error> Failed to receive heartbeat response from Huawei Api Server.");
        log("<error> Failed to receive heartbeat response from Huawei Api Server.");
    }

    if (heartbeat_timer_ != nullptr)
    {
        StartHeartbeat();
    }
}

huawei::api::ErrorCode HuaweiApiClient::ApplyQoSResponse(const google::protobuf::Message& message) {
    if (qos_state_ != kApplyingQosRequest)
    {
        return ErrorCode::ERROR_CODE_UNKNOWN;
    }

    if (message.GetTypeName() != "huawei.api.ApplyQosResponse")
    {
        return ErrorCode::ERROR_CODE_INVALID_MSG;
    }

    const huawei::api::ApplyQosResponse& apply_qos_response = (huawei::api::ApplyQosResponse&)(message);
    if (apply_qos_response.error_code() == huawei::api::ErrorCode::ERROR_CODE_NONE)
    {
        signal_(0, "ApplyQoSRequest has successed.");
        log("ApplyQoSRequest has successed.");

        StopQosRequestTimer();

        qos_state_ = kUnderQosService;

        StartHeartbeat();
    }
    else
    {
        // qos_state_ = kStoppedQosService;

        signal_(-1, boost::str(boost::format(
            "<error> Failed to ApplyQoSRequest. %1%") % 
            apply_qos_response.reason()).c_str());
        log(boost::str(boost::format(
            "<error> Failed to ApplyQoSRequest. %1%") %
            apply_qos_response.reason()).c_str());

        // DoApplyQosRequest();
    }

    return apply_qos_response.error_code();
}

huawei::api::ErrorCode HuaweiApiClient::RemoveQoSResponse(const google::protobuf::Message& message) {
    if (qos_state_ != kRemovingQosRequest)
    {
        return ErrorCode::ERROR_CODE_UNKNOWN;
    }

    if (message.GetTypeName() != "huawei.api.RemoveQosResponse")
    {
        return ErrorCode::ERROR_CODE_INVALID_MSG;
    }

    StopQosRequestTimer();

    // Anyway, set the qos state to stopped.
    qos_state_ = kStoppedQosService;

    const huawei::api::RemoveQosResponse& remove_qos_response = (huawei::api::RemoveQosResponse&)(message);
    if (remove_qos_response.error_code() == huawei::api::ErrorCode::ERROR_CODE_NONE)
    {
        signal_(0, "RemoveQoSRequest has successed.");
        log("RemoveQoSRequest has successed.");
    }
    else
    {
        signal_(-1, boost::str(boost::format(
            "<error> Failed to RemoveQoSRequest. %1%") %
            remove_qos_response.reason()).c_str());
        log(boost::str(boost::format(
            "<error> Failed to RemoveQoSRequest. %1%") %
            remove_qos_response.reason()).c_str());
    }

    return remove_qos_response.error_code();
}

huawei::api::ErrorCode HuaweiApiClient::ReplyHeartbeatResponse(const google::protobuf::Message& message) {
    if (!is_heartbeat_pendding)
    {
        return ErrorCode::ERROR_CODE_UNKNOWN;
    }

    if (message.GetTypeName() != "huawei.api.HeartbeatResponse")
    {
        return ErrorCode::ERROR_CODE_INVALID_MSG;
    }

    is_heartbeat_pendding = false;

    return huawei::api::ErrorCode::ERROR_CODE_NONE;
}

// Jiang's
void
HuaweiApiClient::set_log_file(const std::string& path)
{
    try
    {
        log_file_.reset(new std::fstream(path.c_str(), std::ios::out));

        if (!log_file_->is_open())
        {
            log_file_.reset();
            log("failed to open log file '%1%'", path);
        }
    }
    catch (std::exception& e)
    {
        log("set log exception %1%", e.what());
    }
    catch (...)
    {
        log("set log exception");
    }
}

template<typename... Args>
void
HuaweiApiClient::log(const char* s, Args... args)
{
    try
    {
        boost::format f(s);
        bindArgs(f, args...);
        log(f.str().c_str());
    }
    catch (...)
    {
    }
}

void
HuaweiApiClient::log(const char* s)
{
    try
    {
        std::string now = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());

        boost::mutex::scoped_lock lock(log_mutex);

        if (log_file_)
        {
            *log_file_ << "[" << now << "] " << s << std::endl;
            log_file_->flush();
        }
    }
    catch (...)
    {

    }
}

// ----------------------------------
void* huawei_api_client_create(const char* address, const char* port)
{
    try
    {
        return new HuaweiApiClient(address, port);
    }
    catch (...)
    {
        return nullptr;
    }
}

void huawei_api_client_destory(void* instance)
{
    if (instance)
    {
        delete ((HuaweiApiClient*)instance);
    }
}

void huawei_api_client_set_log_callback(void* instance, huawei_api_client_log_callback_t log_callback)
{
    if (instance)
    {
        ((HuaweiApiClient*)instance)->RegisterCallback(log_callback);
    }
}

void huawei_api_client_set_log_file(void* instance, const char* filename)
{
    if (instance)
    {
        ((HuaweiApiClient*)instance)->set_log_file(filename);
    }
}

void huawei_api_client_start(void* instance)
{
    if (instance)
    {
        ((HuaweiApiClient*)instance)->Start();
    }
}

void huawei_api_client_stop(void* instance)
{
    if (instance) {
        ((HuaweiApiClient*)instance)->Stop();
    }
}
