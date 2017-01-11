#ifndef __NETSTAT_TTCPLIB_TTCP__
#define __NETSTAT_TTCPLIB_TTCP__

#include <string>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

namespace ttcp
{
#define BUFF_SIZE 8192

    typedef boost::function<void(const std::string&)> ReportCallback;

    class TTcpClient
    {
    public:
        TTcpClient(const std::string& dstIp, unsigned short dstPort, ReportCallback callback);
        ~TTcpClient();

        void HandleConnect(const boost::system::error_code& error);
        void HandleWrite(const boost::system::error_code& error);

        void Start();
        void Stop();

        void Run();

    private:
        std::string    m_DstIp;
        unsigned short m_DstPort;
        boost::asio::io_service m_IOservice;
        boost::asio::ip::tcp::socket m_Socket;
        boost::mutex m_Mutex;
        boost::thread* m_Thread;
        ReportCallback m_Callback;
        char m_buff[BUFF_SIZE];
        int m_Times;
        int m_CurrentTimes;
        double m_NumBytes;

        boost::chrono::milliseconds sumGlobal;
        boost::chrono::high_resolution_clock::time_point t1;
        boost::chrono::high_resolution_clock::time_point t2;
    };
}

#endif // __NETSTAT_TTCPLIB_TTCP__