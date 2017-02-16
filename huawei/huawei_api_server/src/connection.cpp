#include "connection.h"
#include "logger.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

using namespace huawei_api_server;

Connection::ConnectionList Connection::s_ConnectionList;

ConnectionPtr
Connection::Create(boost::asio::io_service& IOService)
{
    ConnectionPtr conn(new Connection(IOService));
    s_ConnectionList.push_front(conn);
    return conn;
}

Connection::Connection(boost::asio::io_service& IOService)
    : m_Socket(IOService)
{

}

Connection::~Connection()
{

}

boost::asio::ip::tcp::socket& Connection::GetSocket()
{
    return m_Socket;
}

void Connection::HuaweiApiCallback(int result, const std::string& message) {

}

void Connection::Start()
{
    // Save address and port.
    remote_public_ip_ = m_Socket.remote_endpoint().address().to_string();

    // Set no delay flag.
    boost::system::error_code ignored_ec;
    m_Socket.set_option(boost::asio::ip::tcp::no_delay(true), ignored_ec);
    if (ignored_ec)
    {
        SERVER_LOGGER(warning) << "Failed to set no_delay of socket [" << remote_public_ip_ << "], because of " << ignored_ec << ".";
    }

    const char* realm = "ChangyouRealm";
    const char* username = "ChangyouDevice";
    const char* password = "Changyou@123";
    const char* nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";

    huawei_api_ = new HuaweiAPI(realm, username, password, nonce);
    huawei_api_->RegisterCallback(boost::bind(&Connection::HuaweiApiCallback, this, _1, _2));

    m_RevBuff = { '\n' };

    // OK, it's time to read data.
    m_Socket.async_read_some(boost::asio::buffer(m_RevBuff),
        boost::bind(&Connection::HandleRead, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void Connection::HandleRead(const boost::system::error_code& err,
    std::size_t bytes_transferred)
{
    if (!err)
    {
        std::string local_ip(m_RevBuff.begin(), m_RevBuff.end());
        if (remote_local_ip_ != local_ip)
        {
            remote_local_ip_ = local_ip;
            huawei_api_->AsyncApplyQoSResourceRequest(remote_local_ip_, remote_public_ip_);
        }

        m_RevBuff = { '\n' };

        // Read again and again...
        m_Socket.async_read_some(boost::asio::buffer(m_RevBuff),
            boost::bind(&Connection::HandleRead, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        // Received EOF, close connection.
        // TODO: remove qos request
        Close();
    }
}

void
Connection::Close()
{
    if (m_Socket.is_open())
    {
        SERVER_LOGGER(info) << "Connection [" << remote_public_ip_ << "] has closed.";

        boost::system::error_code ignored_ec;
        m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        if (ignored_ec)
        {
            SERVER_LOGGER(warning) << "Failed to shutdown socket of [" << remote_public_ip_ << "], because of " << ignored_ec << ".";
        }
    }
    s_ConnectionList.remove(shared_from_this());
}
