#ifndef __NETSTAT_TTCP_TTCPSERVER__
#define __NETSTAT_TTCP_TTCPSERVER__

#include <string>
#include <boost/asio.hpp>

namespace ttcp
{
    class TTcpServer
    {
    public:
        TTcpServer(boost::asio::io_service& IOService){}
        ~TTcpServer(){}
    };
}

#endif // __NETSTAT_TTCP_TTCPSERVER__