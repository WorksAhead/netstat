#include "ttcpclient.h"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/date_time.hpp>

// interface
typedef void(*ttcp_client_log_callback_t)(const char*);

void* ttcp_client_create(const char* address, const char* port, uint32_t notifyInterval);
void ttcp_client_destory(void* instance);

void ttcp_client_set_log_callback(void* instance, ttcp_client_log_callback_t log_callback);
void ttcp_client_set_log_file(void* instance, const char* filename);

void ttcp_client_start(void* instance);
void ttcp_client_stop(void* instance);
// end

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

using namespace ttcp;
using boost::asio::ip::tcp;

TTcpClient::TTcpClient(const std::string& address, const std::string& port, uint32_t notifyInterval)
    : m_Addr(address)
    , m_Port(port)
    , m_Socket{m_IOservice}
    , m_NotifyInterval{notifyInterval}
{
    m_Thread = new boost::thread(&TTcpClient::Run, boost::ref(*this));
}

TTcpClient::~TTcpClient()
{
    if (connection_state_ == kConnected)
    {
        boost::system::error_code ignored_ec;
        m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
        m_Socket.close();
    }

    m_IOservice.stop();

    m_Thread->join();
    delete m_Thread;
}

boost::signals2::connection
TTcpClient::RegisterCallback(const SignalType::slot_type& subscriber)
{
    return m_Signal.connect(subscriber);
}

void
TTcpClient::HandleNotifySubscribers(const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted)
    {
        // Timer was not cancelled.
        return;
    }

    if (connection_state_ != kConnected && !m_Stop)
    {
        try {
          // Connect to server.
          Connect();
          return;
        }
        catch (boost::system::system_error& error) {
          m_Signal(boost::str(boost::format(
            "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());
          log(boost::str(boost::format(
            "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());

          m_Stop = true;
        }
    }

    // Reset timer again.
    if (!m_Stop)
    {
        m_Signal(NotifyMsg.c_str());
        log(NotifyMsg.c_str());

        m_NotifyTimer->expires_at(m_NotifyTimer->expires_at() + boost::posix_time::millisec(m_NotifyInterval));
        m_NotifyTimer->async_wait(boost::bind(&TTcpClient::HandleNotifySubscribers, this, boost::asio::placeholders::error));
    }
}

void
TTcpClient::HandleConnect(const boost::system::error_code& error)
{
    if (!error)
    {
        connection_state_ = kConnected;

        m_NotifyTimer.reset(new boost::asio::deadline_timer(m_IOservice, boost::posix_time::millisec(m_NotifyInterval)));
        m_NotifyTimer->async_wait(boost::bind(&TTcpClient::HandleNotifySubscribers, this, boost::asio::placeholders::error));

        SendData();
    }
    else
    {
        m_Signal("<error> Unable to connect to ttcpserver.");
        log("<error> Unable to connect to ttcpserver.");

        connection_state_ = kDisconnected;

        if (!m_Stop)
        {
            m_NotifyTimer.reset(new boost::asio::deadline_timer(m_IOservice, boost::posix_time::millisec(m_NotifyInterval)));
            m_NotifyTimer->async_wait(boost::bind(&TTcpClient::HandleNotifySubscribers, this, boost::asio::placeholders::error));
        }
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

        if (!m_Stop)
        {
          // Send packet again.
          SendData();
        }
    }
    else
    {
        if (!m_Stop)
        {
            m_Signal("<error> Connection disconnected.");
            log("<error> Connection disconnected.");

            try {
              // Connect to server.
              Connect();
            }
            catch (boost::system::system_error& error) {
              m_Signal(boost::str(boost::format(
                "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());
              log(boost::str(boost::format(
                "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());

              m_Stop = true;
            }
        }
    }
}

void
TTcpClient::Start()
{
    if (!m_Stop)
    {
        m_Signal("<error> Failed to start TTcpClient, TTcpClient is running.");
        log("<error> Failed to start TTcpClient, TTcpClient is running.");
        return;
    }

    m_Signal("Start TTcpClient.");
    log("Start TTcpClient.");

    // Reset data.
    m_TotalWriteBytes = 0;

    m_PacketBeginWrite = boost::chrono::high_resolution_clock::now();
    m_PacketEndWrite = boost::chrono::high_resolution_clock::now();
    m_TotalWriteTime = boost::chrono::duration<float>::zero();

    m_Stop = false;

    if (connection_state_ == kDisconnected)
    {
        try {
          // Connect to server.
          Connect();

        } catch (boost::system::system_error& error) {
          m_Signal(boost::str(boost::format(
            "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());
          log(boost::str(boost::format(
            "<error> Failed to connect to TTcpServer. %1%") % error.what()).c_str());

          m_Stop = true;
        }
    }
    else
    {
        SendData();
    }
}

void
TTcpClient::Stop()
{
    m_Signal("Stop TTcpClient.");
    log("Stop TTcpClient.");

    m_Stop = true;
}

void
TTcpClient::Run()
{
    boost::asio::io_service::work work(m_IOservice);
    m_IOservice.run();
}

void
TTcpClient::Connect()
{
    m_Signal("Connecting TTcpServer...");
    log("Connecting TTcpServer...");

    connection_state_ = kConnecting;

    tcp::resolver resolver(m_IOservice);
    tcp::resolver::query query(m_Addr, m_Port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::async_connect(m_Socket, endpoint_iterator,
      boost::bind(&TTcpClient::HandleConnect, this,
        boost::asio::placeholders::error));
}

void
TTcpClient::SendData()
{
    std::fill(std::begin(m_SndBuff), std::end(m_SndBuff), 0);

    boost::asio::async_write(m_Socket,
      boost::asio::buffer(m_SndBuff),
      boost::bind(&TTcpClient::HandleWrite, this,
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

// Jiang's
void
TTcpClient::set_log_file(const std::string& path)
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
TTcpClient::log(const char* s, Args... args)
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
TTcpClient::log(const char* s)
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

void* ttcp_client_create(const char* address, const char* port, uint32_t notifyInterval)
{
    try
    {
        return new TTcpClient(address, port, notifyInterval);
    }
    catch (...)
    {
        return nullptr;
    }
}

void ttcp_client_destory(void* instance)
{
    if (instance)
    {
        delete ((TTcpClient*)instance);
    }
}

void ttcp_client_set_log_callback(void* instance, ttcp_client_log_callback_t log_callback)
{
    if (instance)
    {
        ((TTcpClient*)instance)->RegisterCallback(log_callback);
    }
}

void ttcp_client_set_log_file(void* instance, const char* filename)
{
    if (instance)
    {
        ((TTcpClient*)instance)->set_log_file(filename);
    }
}

void ttcp_client_start(void* instance)
{
    if (instance)
    {
        ((TTcpClient*)instance)->Start();
    }
}

void ttcp_client_stop(void* instance)
{
    if (instance) {
        ((TTcpClient*)instance)->Stop();
    }
}
