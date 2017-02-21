#ifndef HUAWEI_API_CLIENT_HUAWEI_API_CLIENT_H_
#define HUAWEI_API_CLIENT_HUAWEI_API_CLIENT_H_

#include <array>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/noncopyable.hpp>

#include "thread_base.h"
#include "message_handler.h"

namespace huawei_api_client
{
    #define BUFFER_SIZE 128

    class HuaweiApiClient : public ThreadBase, 
                            private boost::noncopyable
    {
    public:
        // error_code: 0 is ok, all others are erros.
        // description: description of the error.
        typedef boost::signals2::signal<void(int error_code, const char* description)> SignalType;

        typedef enum {
            kDisconnected = 0,
            kConnecting,
            kConnected,
        } ConnectionState;

        typedef enum {
            kStoppedQosService = 0,
            kApplyingQosRequest,
            kUnderQosService,
            kRemovingQosRequest,
        } QosState;

    public:
        // heartbeat_interval's unit is seconds.
        HuaweiApiClient(const std::string& address, const std::string& port);
        ~HuaweiApiClient();

        boost::signals2::connection RegisterCallback(const SignalType::slot_type& subscriber);

        void Start();
        void Stop();

        // Jiang's
        void set_log_file(const std::string& path);

    private:
        virtual void Run();

        // Register qos and heartbeat response handler.
        void RegisterResponseHandler();

        // Connect to huawei api server.
        void Connect();
        // Disconnect to huawei api server.
        void Close();

        // Send apply qos request.
        void DoApplyQosRequest();
        // Send remove qos request.
        void DoRemoveQosRequest();

        // Heartbeat.
        void StartHeartbeat();
        void StopHeartbeat();

        // QoS timer
        void StartQosRequestTimer();
        void StopQosRequestTimer();

        // Socket handler.
        void HandleConnect(const boost::system::error_code& error);
        void HandleWrite(const boost::system::error_code& error, std::size_t bytes_transferred);
        void HandleRead(const boost::system::error_code& error, std::size_t bytes_transferred);

        // Timer handler.
        void HandleQosTimeout();
        void HandleHeartbeat();

        // Response handler.
        huawei::api::ErrorCode ApplyQoSResponse(const google::protobuf::Message& message);
        huawei::api::ErrorCode RemoveQoSResponse(const google::protobuf::Message& message);
        huawei::api::ErrorCode ReplyHeartbeatResponse(const google::protobuf::Message& message);

        // Jiang's
        template<typename... Args>
        void log(const char* s, Args... args);
        void log(const char* s);

    private:
        ConnectionState connection_state_ = kDisconnected;
        QosState qos_state_ = kStoppedQosService;

        bool is_heartbeat_pendding = { false };

        // QoS request timeout timer, unit is seconds.
        uint32_t qos_request_timeout_ = { 10 };
        boost::shared_ptr<boost::asio::deadline_timer> qos_request_timer_;

        // Heartbeat timer, unit is seconds.
        uint32_t heartbeat_interval_ = { 30 };
        boost::shared_ptr<boost::asio::deadline_timer> heartbeat_timer_;

        // Huawei api server ip & port.
        std::string addr_;
        std::string port_;

        // Socket & service.
        boost::asio::io_service io_service_;
        boost::asio::ip::tcp::socket socket_;

        // Subscriber signal.
        SignalType signal_;

        // Send & Recv buffer.
        std::array<char, BUFFER_SIZE> recv_buff_;
        std::array<char, BUFFER_SIZE> send_buff_;

        // Response handler.
        MessageHandler response_handler_;

        // Jiang's
        boost::shared_ptr<std::fstream> log_file_;
        boost::mutex log_mutex;
    };
}

#endif // HUAWEI_API_CLIENT_HUAWEI_API_CLIENT_H_
