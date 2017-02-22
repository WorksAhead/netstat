#ifndef HUAWEI_API_SERVER_MESSAGE_HANDLER_H_
#define HUAWEI_API_SERVER_MESSAGE_HANDLER_H_

#include <boost/function.hpp>
#include "huawei_api_message.pb.h"

namespace huawei_api_server
{
    class Connection;
    class MessageHandler
    {
    public:
        typedef boost::function<void(Connection&, const google::protobuf::Message&)> Handler;

    public:
        static void RegisterMessageHandler();

        static void RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag, const Handler& handler);
        static const Handler* FindHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag);
    
    private:
        static void ApplyQoSRequest(Connection& connection, const google::protobuf::Message& message);
        static void RemoveQoSRequest(Connection& connection, const google::protobuf::Message& message);
        static void ReplyHeartbeatRequest(Connection& connection, const google::protobuf::Message& message);

    private:
        typedef std::map<huawei::api::HuaweiApiMessage::MessageTypeCase, Handler> HandlerMap;
        static HandlerMap handler_map_;
    };
}

#endif