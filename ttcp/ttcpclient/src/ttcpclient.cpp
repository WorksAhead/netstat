#include "ttcpclient.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>

using namespace ttcp;
using boost::asio::ip::tcp;

TTcpClient::TTcpClient(const std::string& address, const std::string& port, uint32_t notifyInterval)
    : m_Addr(address)
    , m_Port(port)
    , m_Socket{m_IOservice}
    , m_NotifyInterval{notifyInterval}
{
    m_NotifyTimer = boost::make_shared<boost::asio::deadline_timer>(m_IOservice, boost::posix_time::millisec(m_NotifyInterval));
    m_NotifyTimer->async_wait(boost::bind(&TTcpClient::HandleNotifySubscribers, this));
}

boost::signals2::connection
TTcpClient::RegisterCallback(const SignalType::slot_type& subscriber)
{
    return m_Signal.connect(subscriber);
}

void
TTcpClient::HandleNotifySubscribers()
{
    m_Signal(NotifyMsg);

    // Reset timer again.
    m_NotifyTimer->expires_at(m_NotifyTimer->expires_at() + boost::posix_time::millisec(m_NotifyInterval));
    m_NotifyTimer->async_wait(boost::bind(&TTcpClient::HandleNotifySubscribers, this));
}

void
TTcpClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        // Reset data.
        m_TotalWriteBytes = 0;

        m_PacketBeginWrite = boost::chrono::high_resolution_clock::now();
        m_PacketEndWrite = boost::chrono::high_resolution_clock::now();
        m_TotalWriteTime = boost::chrono::duration<float>::zero();

        std::fill(std::begin(m_SndBuff), std::end(m_SndBuff), 0);

        boost::asio::async_write(m_Socket,
            boost::asio::buffer(m_SndBuff),
            boost::bind(&TTcpClient::HandleWrite, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
}

void
TTcpClient::HandleWrite(const boost::system::error_code& error, std::size_t bytesTransferred)
{
    if (!error)
    {
        // Collect data.
        m_TotalWriteBytes += bytesTransferred;

        m_PacketEndWrite = boost::chrono::high_resolution_clock::now();
        m_TotalWriteTime = m_PacketEndWrite - m_PacketBeginWrite;

        // Update notify message.
        NotifyMsg = boost::str(boost::format("%1$.2f KB") % (m_TotalWriteBytes / 1024.0 / m_TotalWriteTime.count()));

        // Send packet again.
        boost::asio::async_write(m_Socket,
            boost::asio::buffer(m_SndBuff),
            boost::bind(&TTcpClient::HandleWrite, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
}

void
TTcpClient::Start()
{
    // Connect to server.
    tcp::resolver resolver(m_IOservice);
    tcp::resolver::query query(m_Addr, m_Port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::async_connect(m_Socket, endpoint_iterator,
        boost::bind(&TTcpClient::HandleConnect, this,
            boost::asio::placeholders::error));
}

void
TTcpClient::Stop()
{
    boost::system::error_code ignored_ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    m_Socket.close();
    
    m_IOservice.stop();
}

void
TTcpClient::Run()
{
    if (!m_IOservice.stopped())
    {
        m_IOservice.reset();
    }

    m_IOservice.run();
}