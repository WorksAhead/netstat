#include "connection.h"
#include "logger.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace ttcp;

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
    , m_TotalReadBytes(0)
    , m_TotalWriteBytes(0)
{

}

Connection::~Connection()
{

}

boost::asio::ip::tcp::socket& Connection::GetSocket()
{
    return m_Socket;
}

void Connection::Start()
{
    m_RemoteAddr = m_Socket.remote_endpoint().address().to_string();
    m_RemotePort = boost::lexical_cast<std::string>(m_Socket.remote_endpoint().port());

    m_Socket.async_read_some(boost::asio::buffer(m_Buffer),
        boost::bind(&Connection::HandleRead, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void Connection::HandleRead(const boost::system::error_code& err,
    std::size_t bytes_transferred)
{
    if (!err)
    {
        m_TotalReadBytes += bytes_transferred;

        m_Socket.async_read_some(boost::asio::buffer(m_RevBuff),
            boost::bind(&Connection::HandleRead, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        TTCP_LOGGER(debug) << "Complete reading from [" << m_RemoteAddr << ":" << m_RemotePort << "], total bytes is " << m_TotalReadBytes << ".";
        // Received EOF, close connection.
        Close();
    }

    // If an error occurs then no new asynchronous operations are started. This
    // means that all shared_ptr references to the connection object will
    // disappear and the object will be destroyed automatically after this
    // handler returns. The connection class's destructor closes the socket.
}

void Connection::HandleWrite(const boost::system::error_code& err)
{
    if (!err)
    {
        // Initiate graceful connection closure.
        //boost::system::error_code ignored_ec;
        //socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }
    else
    {
        TTCP_LOGGER(debug) << "write failed.";
    }

    // No new asynchronous operations are started. This means that all shared_ptr
    // references to the connection object will disappear and the object will be
    // destroyed automatically after this handler returns. The connection class's
    // destructor closes the socket.
}

void
Connection::Close()
{
    if (m_Socket.is_open())
    {
        TTCP_LOGGER(info) << "Connection [" << m_RemoteAddr << ":" << m_RemotePort << "] has closed.";

        boost::system::error_code ignored_ec;
        m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        if (ignored_ec)
        {
            TTCP_LOGGER(warning) << "Failed to shutdown socket of [" << m_RemoteAddr << ":" << m_RemotePort << "], because of " << ignored_ec << ".";
        }
    }
    s_ConnectionList.remove(shared_from_this());
}
