#include "connection.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "logger.h"
#include "message_handler.h"

using namespace huawei_api_server;
using namespace huawei::api;

Connection::ConnectionList Connection::connection_list_;

ConnectionPtr Connection::Create(boost::asio::io_service& IOService) {
    ConnectionPtr conn(new Connection(IOService));
    connection_list_.push_front(conn);
    return conn;
}

Connection::Connection(boost::asio::io_service& IOService)
    : socket_(IOService) {
    const char* realm = "ChangyouRealm";
    const char* username = "ChangyouDevice";
    const char* password = "Changyou@123";
    const char* nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";

    huawei_api_ = new HuaweiAPI(realm, username, password, nonce);
}

Connection::~Connection() {
    delete huawei_api_;
}

boost::asio::ip::tcp::socket& Connection::socket() {
    return socket_;
}

void Connection::Start() {
    // Save address and port.
    remote_public_ip_ = socket_.remote_endpoint().address().to_string();

    // Set no delay flag.
    boost::system::error_code ignored_ec;
    socket_.set_option(boost::asio::ip::tcp::no_delay(true), ignored_ec);

    // OK, it's time to read data.
    std::fill(std::begin(recv_buff_), std::end(recv_buff_), 0);
    socket_.async_read_some(boost::asio::buffer(recv_buff_),
        boost::bind(&Connection::HandleRead, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void Connection::HandleRead(const boost::system::error_code& error,
    std::size_t bytes_transferred) {
    if (!error)
    {
        // Parse message.
        HuaweiApiMessage huawei_api_message;
        if (!huawei_api_message.ParseFromArray(recv_buff_.data(), (int)bytes_transferred))
        {
            SERVER_LOGGER(warning) << "Failed to parse received data, close the connection.";
            Close();
            return;
        }

        HuaweiApiMessage::MessageTypeCase sub_message_type = huawei_api_message.message_type_case();
        const google::protobuf::FieldDescriptor* sub_message_descriptor = huawei_api_message.GetDescriptor()->FindFieldByNumber(sub_message_type);
        const google::protobuf::Message& sub_message = huawei_api_message.GetReflection()->GetMessage(huawei_api_message, sub_message_descriptor);

        // Find message handler.
        const MessageHandler::Handler* handler = MessageHandler::FindHandler(sub_message_type);
        if (handler == nullptr)
        {
            SERVER_LOGGER(warning) << "Failed to find a handler for the message " << sub_message_type;
            return;
        }

        // Call handler & procoess message.
        (*handler)(*this, sub_message);

        // Setup read callback again.
        socket_.async_read_some(boost::asio::buffer(recv_buff_),
            boost::bind(&Connection::HandleRead, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        // Received EOF, close connection.
        Close();
    }
}

void Connection::HandleWrite(const boost::system::error_code& error, 
    std::size_t bytes_transferred) {
    if (error)
    {
        SERVER_LOGGER(warning) << "Failed to send message, close the connection.";
        Close();
    }
}

void
Connection::Close()
{
    if (socket_.is_open())
    {
        SERVER_LOGGER(info) << "Connection [" << remote_public_ip_ << "] has closed.";

        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        if (ignored_ec)
        {
            SERVER_LOGGER(warning) << "Failed to shutdown socket of [" << remote_public_ip_ << "], because of " << ignored_ec << ".";
        }
    }
    connection_list_.remove(shared_from_this());
}

// --------------- Redirect QoS request ------------------
void Connection::DoApplyQosRequest(const std::string& remote_local_ip) {
    SERVER_LOGGER(info) << "Forward ApplyQoSResourceRequest from " << remote_public_ip_ << ", and it's local ip is " << remote_local_ip;
    huawei_api_->ApplyQoSResourceRequest(remote_local_ip, remote_public_ip_);
}

void Connection::DoRemoveQosRequest() {
    SERVER_LOGGER(info) << "Forward RemoveQoSResourceRequest from " << remote_public_ip_;
    huawei_api_->RemoveQoSResourceRequest();
}

// ---------------- Replays ------------------
void Connection::ReplyApplyQosRequest() {
    SERVER_LOGGER(info) << "Forward ApplyQoSResourceRequest response to " << remote_public_ip_ << ": " << huawei_api_->description();

    ApplyQosResponse* qos_response = new ApplyQosResponse;
    if (huawei_api_->error_code() != 0)
    {
        qos_response->set_error_code(ErrorCode::ERROR_CODE_APPLY_QOS_FAILED);
    }
    else
    {
        qos_response->set_error_code(ErrorCode::ERROR_CODE_NONE);
    }
    qos_response->set_reason(huawei_api_->description());

    HuaweiApiMessage api_message;
    api_message.set_allocated_apply_qos_response(qos_response);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&Connection::HandleWrite, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Connection::ReplyRemoveQosRequest() {
    SERVER_LOGGER(info) << "Forward RemoveQoSResourceRequest response to " << remote_public_ip_ << ": " << huawei_api_->description();

    RemoveQosResponse* qos_response = new RemoveQosResponse;
    if (huawei_api_->error_code() != 0)
    {
        qos_response->set_error_code(ErrorCode::ERROR_CODE_REMOVE_QOS_FAILED);
    }
    else
    {
        qos_response->set_error_code(ErrorCode::ERROR_CODE_NONE);
    }
    qos_response->set_reason(huawei_api_->description());

    HuaweiApiMessage api_message;
    api_message.set_allocated_remove_qos_response(qos_response);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&Connection::HandleWrite, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void Connection::ReplyHeartbeatRequest() {
    HeartbeatResponse* heartbeat_response = new HeartbeatResponse;

    HuaweiApiMessage api_message;
    api_message.set_allocated_heartbeat_response(heartbeat_response);

    std::fill(std::begin(send_buff_), std::end(send_buff_), 0);
    api_message.SerializeToArray(send_buff_.data(), api_message.ByteSize());

    boost::asio::async_write(socket_,
        boost::asio::buffer(send_buff_.data(), api_message.ByteSize()),
        boost::bind(&Connection::HandleWrite, shared_from_this(),
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
