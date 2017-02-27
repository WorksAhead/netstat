#include "huawei_api_server.h"
#include <boost/bind.hpp>
#include "message_handler.h"
#include "logger.h"

using namespace huawei_api_server;

HuaweiApiServer::HuaweiApiServer(const std::string& address,
                                 const std::string& port,
                                 std::size_t pool_size)
    : io_service_pool_(pool_size)
    , signals_(io_service_pool_.GetIOService())
    , acceptor_(io_service_pool_.GetIOService()) {

    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    m_Signals.add(SIGQUIT);
#endif
    signals_.async_wait(boost::bind(&HuaweiApiServer::HandleStop, this));

    // Register google protobuf message handler.
    MessageHandler::RegisterMessageHandler();

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    StartAccept();
}

HuaweiApiServer::~HuaweiApiServer() {

}

void HuaweiApiServer::Run() {
    io_service_pool_.Run();
}

void HuaweiApiServer::HandleStop() {
    SERVER_LOGGER(info) << "Received QUIT signal.";
    io_service_pool_.Stop();
}

void HuaweiApiServer::StartAccept() {
    ConnectionPtr conn = Connection::Create(io_service_pool_.GetIOService());

    acceptor_.async_accept(conn->socket(),
        boost::bind(&HuaweiApiServer::HandleAccept, this, conn,
            boost::asio::placeholders::error));
}

void HuaweiApiServer::HandleAccept(ConnectionPtr conn, const boost::system::error_code& err) {
    if (!err)
    {
        SERVER_LOGGER(info) << "Accepted a connection from [" << conn->socket().remote_endpoint() << "].";
        conn->Start();
    }
    else
    {
        conn->Close();
    }

    StartAccept();
}
