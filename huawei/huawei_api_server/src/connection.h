#ifndef __NETSTAT_TTCP_CONNECTION__
#define __NETSTAT_TTCP_CONNECTION__

#include <array>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/container/slist.hpp>
#include <boost/chrono.hpp>

#include "huawei_api.h"

namespace huawei_api_server
{
    #define BUFF_SIZE 8192

    class Connection;
    typedef boost::shared_ptr<Connection> ConnectionPtr;

    // Represents a single connection from a client.
    class Connection
        : public boost::enable_shared_from_this<Connection>,
        private boost::noncopyable
    {
    public:
        static ConnectionPtr Create(boost::asio::io_service& IOService);

        // Construct a connection with the given io_service.
        explicit Connection(boost::asio::io_service& IOService);
        ~Connection();

        // Get the socket associated with the connection.
        boost::asio::ip::tcp::socket& GetSocket();

        // Start the first asynchronous operation for the connection.
        void Start();
        // Close the connection and remove itself from s_ConnectionList.
        void Close();

    private:
        // Handle completion of a read operation.
        void HandleRead(const boost::system::error_code& err, std::size_t bytes_transferred);

        //
        void HuaweiApiCallback(int result, const std::string& message);

    private:
        // Socket for the connection.
        boost::asio::ip::tcp::socket m_Socket;

        // Remote address info.
        std::string remote_public_ip_;
        std::string remote_local_ip_;

        std::array<char, BUFF_SIZE> m_RevBuff;

        HuaweiAPI* huawei_api_;

        // Global activity connections.
        typedef boost::container::slist<ConnectionPtr> ConnectionList;
        static ConnectionList s_ConnectionList;
    };
}

#endif // __NETSTAT_TTCP_CONNECTION__
