#ifndef HUAWEI_API_SERVER_HUAWEI_API_H_
#define HUAWEI_API_SERVER_HUAWEI_API_H_

#include <string>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <curl/curl.h>

namespace huawei_api_server
{
    #define QOS_RESOURCE_REQUEST_URL "http://183.207.208.184/services/QoSV1/DynamicQoS"

    class HuaweiAPI
    {
    public:
        typedef boost::signals2::signal<void(int, const std::string&)> SignalType;

    public:
        HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce);
        ~HuaweiAPI();

        // Register callback functions.
        boost::signals2::connection RegisterCallback(const SignalType::slot_type& subscriber);

        // Apply QoSResourceRequest.
        void AsyncApplyQoSResourceRequest(const std::string& local_ip, const std::string& public_ip);
        void ApplyQoSResourceRequest(const std::string& local_ip, const std::string& public_ip);

        // Stop QoSResourceRequest.
        void AsyncRemoveQoSResourceRequest();
        void RemoveQoSResourceRequest();

        const SignalType& signal()
        {
            return signal_;
        }

        void set_correlation_id(const std::string& correlation_id)
        {
            correlation_id_ = correlation_id;
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
        std::string m_Realm;
        std::string m_Username;
        std::string m_Password;
        std::string m_Nonce;

        // CorrelationId. uniquely specify a QoS request.
        std::string correlation_id_;

        // Callback signal object.
        SignalType signal_;

        boost::shared_ptr<boost::thread> m_Thread;
    };
}

#endif // HUAWEI_API_SERVER_HUAWEI_API_H_
