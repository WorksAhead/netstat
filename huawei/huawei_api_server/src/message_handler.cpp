#include "message_handler.h"
#include "Connection.h"
#include "logger.h"

using namespace huawei_api_server;

MessageHandler::HandlerMap MessageHandler::handler_map_;

void MessageHandler::RegisterMessageHandler() {
    RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase::kApplyQosRequest, ApplyQoSRequest);
    RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase::kRemoveQosRequest, RemoveQoSRequest);
    RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase::kHeartbeatRequest, ReplyHeartbeatRequest);
}

void MessageHandler::RegisterHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag, const Handler& handler) {
    handler_map_[message_tag] = handler;
}

const MessageHandler::Handler* MessageHandler::FindHandler(huawei::api::HuaweiApiMessage::MessageTypeCase message_tag) {
    HandlerMap::const_iterator itor = handler_map_.find(message_tag);
    if (itor != handler_map_.end())
    {
        return &(itor->second);
    }

    return nullptr;
}

void MessageHandler::ApplyQoSRequest(Connection& connection, const google::protobuf::Message& message) {
    if (message.GetTypeName() != "huawei.api.ApplyQosRequest")
    {
        SERVER_LOGGER(warning) << "Message type is incorrect." << std::endl;
        return;
    }

    const huawei::api::ApplyQosRequest& apply_qos_request = (huawei::api::ApplyQosRequest&)(message);

    connection.DoApplyQosRequest(apply_qos_request.local_ip());
    connection.ReplyApplyQosRequest();
}

void MessageHandler::RemoveQoSRequest(Connection& connection, const google::protobuf::Message& message) {
    if (message.GetTypeName() != "huawei.api.RemoveQosRequest")
    {
        SERVER_LOGGER(warning) << "Message type is incorrect." << std::endl;
        return;
    }

    const huawei::api::RemoveQosRequest& remove_qos_request = (huawei::api::RemoveQosRequest&)(message);

    connection.DoRemoveQosRequest();
    connection.ReplyRemoveQosRequest();
}

void MessageHandler::ReplyHeartbeatRequest(Connection& connection, const google::protobuf::Message& message) {
    if (message.GetTypeName() != "huawei.api.HeartbeatRequest")
    {
        SERVER_LOGGER(warning) << "Message type is incorrect." << std::endl;
        return;
    }

    connection.ReplyHeartbeatRequest();
}