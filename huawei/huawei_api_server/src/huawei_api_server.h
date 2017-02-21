#ifndef HUAWEI_API_SERVER_HUAWEI_API_SERVER_H_
#define HUAWEI_API_SERVER_HUAWEI_API_SERVER_H_

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include "io_service_pool.h"
#include "connection.h"

namespace huawei_api_server
{
    class HuaweiApiServer : private boost::noncopyable
    {
    public:
        HuaweiApiServer(const std::string& address,
                        const std::string& port,
                        std::size_t pool_size);
        ~HuaweiApiServer();

        // Run the server's io_service loop.
        void Run();

    private:
        // Initiate an asynchronous accept operation.
        void StartAccept();

        // Handle a request to stop the server.
        void HandleStop();
        // Handle completion of an asynchronous accept operation.
        void HandleAccept(ConnectionPtr conn, const boost::system::error_code& err);

    private:
        // The pool of io_service objects used to perform asynchronous operations.
        IOServicePool m_IOServicePool;

        // The signal_set is used to register for process termination notifications.
        boost::asio::signal_set m_Signals;
        // Acceptor used to listen for incoming connections.
        boost::asio::ip::tcp::acceptor m_Acceptor;
    };
}

#endif // HUAWEI_API_SERVER_HUAWEI_API_SERVER_H_
