#ifndef __NETSTAT_TTCP_TTCPCLIENT__
#define __NETSTAT_TTCP_TTCPCLIENT__

#include <string>
#include <array>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

namespace ttcp
{
    #define BUFF_SIZE 8192

    class TTcpClient
    {
    public:
        typedef boost::signals2::signal<void(const char*)> SignalType;

    public:
        // @notifyInterval: Callback interval in milliseconds.
        TTcpClient(const std::string& address, const std::string& port, uint32_t notifyInterval = 1000);

        boost::signals2::connection RegisterCallback(const SignalType::slot_type& subscriber);

        void Start();
        void Stop();

        // This function would block the current thread that has invoked it.
        void Run();

        // Jiang's
        void set_log_file(const std::string& path);

    private:
        void HandleNotifySubscribers();

        void HandleConnect(const boost::system::error_code& error);
        void HandleWrite(const boost::system::error_code& error, std::size_t bytesTransferred);

        // Jiang's
        template<typename... Args>
        void log(const char* s, Args... args);
        void log(const char* s);

    private:
        bool m_Stop{false};

        std::string m_Addr;
        std::string m_Port;

        boost::asio::io_service m_IOservice;
        boost::asio::ip::tcp::socket m_Socket;
        boost::shared_ptr<boost::thread> m_Thread;

        // Notify the subscriber every 1000 msec during client is running as default.
        SignalType m_Signal;
        std::string NotifyMsg;
        uint32_t m_NotifyInterval;
        boost::shared_ptr<boost::asio::deadline_timer> m_NotifyTimer;

        std::array<char, BUFF_SIZE> m_RevBuff;
        std::array<char, BUFF_SIZE> m_SndBuff;

        // Write data.
        double m_TotalWriteBytes;
        boost::chrono::duration<float> m_TotalWriteTime;
        boost::chrono::high_resolution_clock::time_point m_PacketBeginWrite;
        boost::chrono::high_resolution_clock::time_point m_PacketEndWrite;

        // Jiang's
        boost::shared_ptr<std::fstream> log_file_;
        boost::mutex log_mutex;
    };
}

#endif // __NETSTAT_TTCP_TTCPCLIENT__
