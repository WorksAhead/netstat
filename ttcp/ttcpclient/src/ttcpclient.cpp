#include "ttcpclient.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>

using namespace ttcp;
using boost::asio::ip::tcp;

TTcpClient::TTcpClient(const std::string& address, const std::string& port, uint32_t notifyInterval)
    : m_Socket(m_IOservice)
    , m_NotifyInterval{notifyInterval}
{

    m_NotifyTimer = boost::make_shared<boost::asio::deadline_timer>(m_IOservice, boost::posix_time::millisec(m_NotifyInterval));
    m_NotifyTimer->async_wait(boost::bind(&TTcpClient::NotifySubscribers, this));

    tcp::resolver resolver(m_IOservice);
    tcp::resolver::query query(address, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::async_connect(m_Socket, endpoint_iterator,
        boost::bind(&TTcpClient::HandleConnect, this,
            boost::asio::placeholders::error));
}

TTcpClient::~TTcpClient()
{

}

boost::signals2::connection
TTcpClient::RegisterCallback(const SignalType::slot_type& subscriber)
{
    return m_Signal.connect(subscriber);
}

void
TTcpClient::NotifySubscribers()
{
    m_Signal(NotifyMsg);

    // Wait again.
    m_NotifyTimer->expires_at(m_NotifyTimer->expires_at() + boost::posix_time::millisec(m_NotifyInterval));
    m_NotifyTimer->async_wait(boost::bind(&TTcpClient::NotifySubscribers, this));
}

void
TTcpClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
    }
}

void
TTcpClient::HandleWrite(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        m_PacketEndWrite = boost::chrono::high_resolution_clock::now();

        m_TotalWriteTime = (boost::chrono::duration_cast<boost::chrono::microseconds>(m_PacketEndWrite - m_PacketBeginWrite));

        m_TotalWriteBytes += bytes_transferred;

        typedef boost::chrono::duration<float> seconds;
        seconds sec = m_TotalWriteTime;
        //std::cout << "time of sec : " << sec.count() << ", msec : " << m_TotalWriteTime << std::endl;
        NotifyMsg = boost::str(boost::format("%1$.2f KB") % (m_TotalWriteBytes / 1024.0 / sec.count()));

        /*
        m_CurrentTimes++;
        if (m_CurrentTimes == m_Times)
        {
            boost::system::error_code ignored_ec;
            m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            m_IOservice.stop();

            std::cout << "time cost msec is " << m_TotalSendTime << " and sec is " << sec << std::endl;
            std::string kb = boost::str(boost::format("%1$.2f KB") % (m_TotalSendBytes / sec.count() / 1024.0));
            std::cout << "trans rate is " << kb << std::endl;
        }
        */

        //m_PacketBeginWrite = boost::chrono::high_resolution_clock::now();

        boost::asio::async_write(m_Socket,
            boost::asio::buffer(m_SndBuff, BUFF_SIZE),
            boost::bind(&TTcpClient::HandleWrite, this,
                boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
}

void
TTcpClient::Start()
{
    m_TotalWriteBytes = 0;
    m_PacketBeginWrite = boost::chrono::high_resolution_clock::now();

    std::fill(std::begin(m_SndBuff), std::end(m_SndBuff), 0);

    boost::asio::async_write(m_Socket,
        boost::asio::buffer(m_SndBuff, BUFF_SIZE),
        boost::bind(&TTcpClient::HandleWrite, this,
            boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

}

void
TTcpClient::Stop()
{
    m_IOservice.stop();
}

void
TTcpClient::Run()
{
    m_IOservice.run();
}