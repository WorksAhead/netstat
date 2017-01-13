#include "ttcpserver.h"
#include "logger.h"
#include <boost/bind.hpp>

using namespace ttcp;

TTcpServer::TTcpServer(const std::string& address,
                       const std::string& port,
                              std::size_t pool_size)
    : m_IOServicePool(pool_size)
    , m_Signals(m_IOServicePool.GetIOService())
    , m_Acceptor(m_IOServicePool.GetIOService())
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    m_Signals.add(SIGINT);
    m_Signals.add(SIGTERM);
#if defined(SIGQUIT)
    m_Signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
    m_Signals.async_wait(boost::bind(&TTcpServer::HandleStop, this));

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(m_Acceptor.get_io_service());
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    m_Acceptor.open(endpoint.protocol());
    m_Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    m_Acceptor.bind(endpoint);
    m_Acceptor.listen();

    StartAccept();
}

TTcpServer::~TTcpServer()
{

}

void
TTcpServer::Run()
{
    m_IOServicePool.Run();
}

void
TTcpServer::HandleStop()
{
    TTCP_LOGGER(info) << "Received QUIT signal.";
    m_IOServicePool.Stop();
}

void
TTcpServer::StartAccept()
{
    ConnectionPtr conn = Connection::Create(m_IOServicePool.GetIOService());

    m_Acceptor.async_accept(conn->GetSocket(),
        boost::bind(&TTcpServer::HandleAccept, this, conn,
            boost::asio::placeholders::error));
}

void
TTcpServer::HandleAccept(ConnectionPtr conn, const boost::system::error_code& err)
{
    if (!err)
    {
        TTCP_LOGGER(info) << "Accepted a connection from [" << conn->GetSocket().remote_endpoint() << "].";
        conn->Start();
    }
    else
    {
        TTCP_LOGGER(warning) << "Failed to accept connection from [" << conn->GetSocket().remote_endpoint() << "].";
        conn->Close();
    }

    StartAccept();
}
