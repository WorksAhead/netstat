#ifndef HUAWEI_API_SERVER_CONNECTION_H_
#define HUAWEI_API_SERVER_CONNECTION_H_

#include <array>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/container/slist.hpp>

#include "huawei_api.h"
#include "message_handler.h"

namespace huawei_api_server
{
    #define BUFFER_SIZE 256

    class Connection;
    typedef boost::shared_ptr<Connection> ConnectionPtr;

    // Represents a single connection from a client.
    class Connection
        : public boost::enable_shared_from_this<Connection>,
        private boost::noncopyable
    {
    public:
        // Create a connection.
        static ConnectionPtr Create(boost::asio::io_service& IOService);
        // QoS Request message handler.
        //static void RegisterMessageHandler();

        // Construct a connection with the given io_service.
        explicit Connection(boost::asio::io_service& IOService);
        ~Connection();

        // Get the socket associated with the connection.
        boost::asio::ip::tcp::socket& socket();

        // Start the first asynchronous operation for the connection.
        void Start();
        // Close the connection and remove itself from s_ConnectionList.
        void Close();

        // Redirect QoS Request.
        void DoApplyQosRequest(const std::string& remote_local_ip);
        void DoRemoveQosRequest();
        
        void ReplyApplyQosRequest();
        void ReplyRemoveQosRequest();
        void ReplyHeartbeatRequest();



    private:
        // Handle completion of a read operation.
        void HandleRead(const boost::system::error_code& err, std::size_t bytes_transferred);
        void HandleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);

        void DoApplyQoSResponse(int error_code, const std::string& description);

        //
        //static void ApplyQoSRequest(Connection& connection, const google::protobuf::Message& message);
        //static void RemoveQoSRequest(Connection& connection, const google::protobuf::Message& message);
        //static void ReplyHeartbeatRequest(Connection& connection, const google::protobuf::Message& message);

    private:
        // Socket for the connection.
        boost::asio::ip::tcp::socket socket_;

        // Send & Recv buffer.
        std::array<char, BUFFER_SIZE> recv_buff_;
        std::array<char, BUFFER_SIZE> send_buff_;

        //
        HuaweiAPI* huawei_api_;

        std::string remote_public_ip_;

        // Global activity connections.
        typedef boost::container::slist<ConnectionPtr> ConnectionList;
        static ConnectionList s_ConnectionList;

        // Message handler.
        //static MessageHandler message_handler_;
    };
}

#endif // HUAWEI_API_SERVER_CONNECTION_H_
