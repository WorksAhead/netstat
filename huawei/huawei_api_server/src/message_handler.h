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
        typedef boost::function<huawei::api::ErrorCode(Connection&, const google::protobuf::Message&)> Handler;

    public:
        void RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag, const Handler& handler)
        {
            handler_map_[message_tag] = handler;
        }

        Handler GetHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag)
        {
            return handler_map_[message_tag];
        }

    private:
        std::map<huawei::api::HuaweiApiMessage::MessageTypeCase, Handler> handler_map_;
    };
}

#endif