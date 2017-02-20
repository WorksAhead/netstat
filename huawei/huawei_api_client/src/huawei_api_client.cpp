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

void huawei_api_start(void* instance);
void huawei_api_stop(void* instance);
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

}

HuaweiApiClient::~HuaweiApiClient() {
    if (socket_.is_open()) {
        Close();
    }

    signal_.disconnect_all_slots();

    io_service_.stop();
}

boost::signals2::connection HuaweiApiClient::RegisterCallback(
    const SignalType::slot_type& subscriber) {
    return signal_.connect(subscriber);
}

void HuaweiApiClient::Start() {
    if (socket_.is_open()) {
        Close();
    }

    Connect();
}

void HuaweiApiClient::Stop() {
    DoRemoveQosRequest();

    qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
    qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this));
}

void HuaweiApiClient::Run()
{
    boost::asio::io_service::work work(io_service_);
    io_service_.run();
}

void HuaweiApiClient::RegisterResponseHandler()
{
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kApplyQosRequest, boost::bind(&HuaweiApiClient::ApplyQoSResponse, this, _1));
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kRemoveQosRequest, boost::bind(&HuaweiApiClient::RemoveQoSResponse, this, _1));
    response_handler_.RegisterHandler(HuaweiApiMessage::MessageTypeCase::kHeartbeatRequest, boost::bind(&HuaweiApiClient::ReplyHeartbeatResponse, this, _1));
}

void HuaweiApiClient::Connect() {
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(addr_, port_);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::async_connect(socket_, endpoint_iterator,
        boost::bind(&HuaweiApiClient::HandleConnect, this,
            boost::asio::placeholders::error));
}

void HuaweiApiClient::Close() {
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
}

void HuaweiApiClient::DoApplyQosRequest() {
    is_apply_qos_pendding = true;

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
    is_remove_qos_pendding = true;

    RemoveQosRequest* qos_request = new RemoveQosRequest;

    HuaweiApiMessage api_message;
    api_message.set_allocated_remove_qos_request(qos_request);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), (int)send_buff_.size());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_),
        boost::bind(&HuaweiApiClient::HandleWrite, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void HuaweiApiClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        DoApplyQosRequest();

        qos_request_timer_.reset(new boost::asio::deadline_timer(io_service_, boost::posix_time::seconds(qos_request_timeout_)));
        qos_request_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleQosTimeout, this));
    }
    else
    {
        signal_(-1, "<error> Failed to connect to Huawei Api Server.");
        log("<error> Failed to connect to Huawei Api Server.");
    }
}

void HuaweiApiClient::HandleWrite(const boost::system::error_code& error, std::size_t bytesTransferred)
{
    if (!error)
    {
        // Ready to read response.
        boost::asio::async_read(socket_,
            boost::asio::buffer(recv_buff_),
            boost::bind(&HuaweiApiClient::HandleRead, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        signal_(-1, "<error> Failed to write data to Huawei Api Server.");
        log("<error> Failed to write data to Huawei Api Server.");
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
    }
    else
    {
        if (is_apply_qos_pendding || is_remove_qos_pendding || is_heartbeat_pendding)
        {
            signal_(-1, "<error> Failed to read data to Huawei Api Server.");
            log("<error> Failed to read data to Huawei Api Server.");
        }
    }
}

void HuaweiApiClient::HandleQosTimeout() {
    if (is_apply_qos_pendding) {
        is_apply_qos_pendding = false;
        signal_(-1, "Apply QoS request timeout.");
    }
    else if (is_remove_qos_pendding) {
        is_remove_qos_pendding = false;
        signal_(-1, "Remove QoS request timeout.");
    }
}

void HuaweiApiClient::HandleHeartbeat()
{
    // Reset timer again.
    if (has_qos_started)
    {
        heartbeat_timer_->expires_at(heartbeat_timer_->expires_at() + boost::posix_time::millisec(heartbeat_interval_));
        heartbeat_timer_->async_wait(boost::bind(&HuaweiApiClient::HandleHeartbeat, this));
    }
}

huawei::api::ErrorCode HuaweiApiClient::ApplyQoSResponse(const google::protobuf::Message& message) {


    return huawei::api::ErrorCode::ERROR_CODE_NONE;
}

huawei::api::ErrorCode HuaweiApiClient::RemoveQoSResponse(const google::protobuf::Message& message) {
    return huawei::api::ErrorCode::ERROR_CODE_NONE;
}

huawei::api::ErrorCode HuaweiApiClient::ReplyHeartbeatResponse(const google::protobuf::Message& message) {
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

void huawei_api_start(void* instance)
{
    if (instance)
    {
        ((HuaweiApiClient*)instance)->Start();
    }
}

void huawei_api_stop(void* instance)
{
    if (instance) {
        ((HuaweiApiClient*)instance)->Stop();
    }
}
