#ifndef HUAWEI_API_SERVER_HUAWEI_API_H_
#define HUAWEI_API_SERVER_HUAWEI_API_H_

#include <string>
#include <boost/signals2.hpp>
#include <curl/curl.h>

namespace huawei_api_server
{
    #define QOS_RESOURCE_REQUEST_URL "http://183.207.208.184/services/QoSV1/DynamicQoS"

    class HuaweiAPI
    {
    public:
        HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce);
        ~HuaweiAPI();

        // Apply QoSResourceRequest.
        void ApplyQoSResourceRequest(const std::string& local_ip, const std::string& public_ip);

        // Stop QoSResourceRequest.
        void RemoveQoSResourceRequest();

        // correlation_id accessor
        void set_correlation_id(const std::string& correlation_id)
        {
            correlation_id_ = correlation_id;
        }
        std::string correlation_id() const
        {
            return correlation_id_;
        }

        // Result code
        void set_error_code(int error_code)
        {
            error_code_ = error_code;
        }
        int error_code() const
        {
            return error_code_;
        }

        // Result description
        void set_description(const std::string& desc)
        {
            description_ = desc;
        }
        std::string description() const
        {
            return description_;
        }

    private:
        // SHA-256 encrypt.
        void Encrypt(const unsigned char* message, unsigned int len, unsigned char* result);

        // Construct Authorization header.
        void AddAuthorizationHeaders(struct curl_slist** headerList);

        // Construct ApplyQoSResourceRequest header, must be called after init curl.
        struct curl_slist* ConstructApplyQoSResourceRequestHeaders();
        // Construct ApplyQoSResourceRequest api body.
        std::string ConstructApplyQoSResourceRequestBody(const std::string& local_ip, const std::string& public_ip);

        // Construct RemoveQoSResourceRequest header.
        struct curl_slist* ConstructRemoveQoSResourceRequestHeaders();

    private:
        std::string realm_;
        std::string username_;
        std::string password_;
        std::string nonce_;

        // CorrelationId. uniquely specify a QoS request.
        std::string correlation_id_;

        // Result
        int error_code_;
        std::string description_;
    };
}

#endif // HUAWEI_API_SERVER_HUAWEI_API_H_
