#include "connection.h"
#include "logger.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

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
    // Reset data.
    m_TotalReadBytes = 0;

    m_PacketBeginRead = boost::chrono::high_resolution_clock::now();
    m_PacketEndRead = boost::chrono::high_resolution_clock::now();
    m_TotalReadTimes = boost::chrono::duration<float>::zero();

    // Save address and port.
    m_RemoteAddr = m_Socket.remote_endpoint().address().to_string();
    m_RemotePort = boost::lexical_cast<std::string>(m_Socket.remote_endpoint().port());

    // Set no delay flag.
    boost::system::error_code ignored_ec;
    m_Socket.set_option(boost::asio::ip::tcp::no_delay(true), ignored_ec);
    if (ignored_ec)
    {
        TTCP_LOGGER(warning) << "Failed to set no_delay of socket [" << m_RemoteAddr << ":" << m_RemotePort << "], because of " << ignored_ec << ".";
    }

    // OK, it's time to read data.
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
        // Collect data.
        m_TotalReadBytes += bytes_transferred;
        m_PacketEndRead = boost::chrono::high_resolution_clock::now();
        m_TotalReadTimes = m_PacketEndRead - m_PacketBeginRead;

        // Read again and again...
        m_Socket.async_read_some(boost::asio::buffer(m_RevBuff),
            boost::bind(&Connection::HandleRead, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        // Received EOF, close connection.
        PrintResult();
        Close();
    }
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

void
Connection::PrintResult()
{
    double totalBps = m_TotalReadBytes / m_TotalReadTimes.count();
    double totalKBps = 0;
    double totalMBps = 0;
    if (totalBps > 1024)
    {
        totalKBps = totalBps / 1024;
    }
    if (totalKBps > 1024)
    {
        totalMBps = totalKBps / 1024;
    }

    std::string outputString;
    if (totalMBps > 0)
    {
        outputString = boost::str(boost::format("Received data %1% MB from [%2%:%3%] in %4% real seconds = %5% MB/sec +++") 
                                                % (m_TotalReadBytes / 1024.0 / 1024.0) % m_RemoteAddr % m_RemotePort % m_TotalReadTimes.count() % totalMBps);
    }
    else if (totalKBps > 0)
    {
        outputString = boost::str(boost::format("Received data %1% KB from [%2%:%3%] in %4% real seconds = %5% KB/sec +++")
            % (m_TotalReadBytes / 1024.0) % m_RemoteAddr % m_RemotePort % m_TotalReadTimes.count() % totalKBps);
    }
    else
    {
        outputString = boost::str(boost::format("Received data %1% Bytes from [%2%:%3%] in %4% real seconds = %5% Bytes/sec +++")
            % m_TotalReadBytes % m_RemoteAddr % m_RemotePort % m_TotalReadTimes.count() % m_TotalReadBytes);
    }
    
    TTCP_LOGGER(debug) << outputString << ".";
}