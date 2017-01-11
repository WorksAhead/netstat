#ifndef __NETSTAT_TTCP_TTCPSERVER__
#define __NETSTAT_TTCP_TTCPSERVER__

#include <string>
#include <boost/asio.hpp>
#include "io_service_pool.h"

namespace ttcp
{
    class TTcpServer
    {
    public:
        TTcpServer(boost::asio::io_service& IOService, unsigned short port);
        ~TTcpServer(){}

        /// The pool of io_service objects used to perform asynchronous operations.
        io_service_pool io_service_pool_;

        /// The signal_set is used to register for process termination notifications.
        boost::asio::signal_set signals_;

        /// Acceptor used to listen for incoming connections.
        boost::asio::ip::tcp::acceptor acceptor_;

        /// The next connection to be accepted.
        connection_ptr new_connection_;
    };
}

#endif // __NETSTAT_TTCP_TTCPSERVER__