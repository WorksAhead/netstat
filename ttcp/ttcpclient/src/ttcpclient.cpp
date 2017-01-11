#include "ttcpclient.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace ttcp;
using boost::asio::ip::tcp;

TTcpClient::TTcpClient(const std::string& dstIp, unsigned short dstPort, ReportCallback callback)
    : m_DstIp(dstIp)
    , m_DstPort(dstPort)
    , m_Socket(m_IOservice)
    , m_Callback(callback)
    , m_NumBytes(0)
{
    memset(m_buff, 'a', BUFF_SIZE);
    m_Times = 2500;
    m_CurrentTimes = 0;
}

TTcpClient::~TTcpClient()
{
    m_Thread->join();
    delete m_Thread;
}

void
TTcpClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        //std::cout << "conn" << std::endl;
        t1 = boost::chrono::high_resolution_clock::now();

        boost::asio::async_write(m_Socket,
            boost::asio::buffer(m_buff, BUFF_SIZE),
            boost::bind(&TTcpClient::HandleWrite, this,
                boost::asio::placeholders::error));
    }
}

void
TTcpClient::HandleWrite(const boost::system::error_code& error)
{
    if (!error)
    {
        t2 = boost::chrono::high_resolution_clock::now();

        sumGlobal += (boost::chrono::duration_cast<boost::chrono::milliseconds>(t2 - t1));

        m_NumBytes += BUFF_SIZE;

        typedef boost::chrono::duration<float> seconds;
        seconds sec = sumGlobal;
        std::string kb = boost::str(boost::format("%1$.2f KB") % (m_NumBytes / sec.count() / 1024.0));
        m_Callback(kb);

        m_CurrentTimes++;
        if (m_CurrentTimes == m_Times)
        {
            boost::system::error_code ignored_ec;
            m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            m_IOservice.stop();

            std::cout << "time cost msec is " << sumGlobal << " and sec is " << sec << std::endl;
            std::string kb = boost::str(boost::format("%1$.2f KB") % (m_NumBytes / sec.count() / 1024.0));
            std::cout << "trans rate is " << kb << std::endl;
        }

        t1 = boost::chrono::high_resolution_clock::now();

        boost::asio::async_write(m_Socket,
            boost::asio::buffer(m_buff, BUFF_SIZE),
            boost::bind(&TTcpClient::HandleWrite, this,
                boost::asio::placeholders::error));
    }
}

void
TTcpClient::Start()
{
    tcp::resolver resolver(m_IOservice);
    tcp::resolver::query query(m_DstIp, boost::lexical_cast<std::string>(m_DstPort));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    

    boost::asio::async_connect(m_Socket, endpoint_iterator,
        boost::bind(&TTcpClient::HandleConnect, this,
            boost::asio::placeholders::error));

    //m_Thread = new boost::thread(boost::bind(&boost::asio::io_service::run, &m_IOservice));
}

void
TTcpClient::Run()
{
    m_IOservice.run();
}