#ifndef __NETSTAT_TTCP_CONNECTION__
#define __NETSTAT_TTCP_CONNECTION__

#include <array>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/container/slist.hpp>

namespace ttcp
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
        // Handle completion of a write operation.
        void HandleWrite(const boost::system::error_code& err);

    private:
        // Socket for the connection.
        boost::asio::ip::tcp::socket m_Socket;

        // Remote address info.
        std::string m_RemoteAddr;
        std::string m_RemotePort;

        // Buffer for incoming data.
        boost::array<char, 8192> m_Buffer;

        std::array<char, BUFF_SIZE> m_RevBuff;
        std::array<char, BUFF_SIZE> m_SndBuff;

        // Totally 
        uint64_t m_TotalReadBytes;
        uint64_t m_TotalWriteBytes;

        typedef boost::container::slist<ConnectionPtr> ConnectionList;
        static ConnectionList s_ConnectionList;
    };
}

#endif // __NETSTAT_TTCP_CONNECTION__
