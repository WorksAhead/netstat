#ifndef __NETSTAT_HUAWEI_API__
#define __NETSTAT_HUAWEI_API__

#include <string>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <curl.h>

namespace huawei
{
    #define QOS_RESOURCE_REQUEST_URL "http://183.207.208.184/services/QoSV1/DynamicQoS"
    #define PUBLIC_IP_QUERY_URL "http://ip.taobao.com/service/getIpInfo2.php?ip=myip&qq-pf-to=pcqq.group"

    class HuaweiAPI
    {
    public:
        typedef boost::signals2::signal<void(int, const char*)> SignalType;

    public:
        HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce);
        ~HuaweiAPI();

        // Register callback functions.
        boost::signals2::connection RegisterCallback(const SignalType::slot_type& subscriber);

        // Apply QoSResourceRequest.
        void AsyncApplyQoSResourceRequest();
        void ApplyQoSResourceRequest();

        // Stop QoSResourceRequest.
        void AsyncRemoveQoSResourceRequest();
        void RemoveQoSResourceRequest();

        // CorrelationId accessor. uniquely specify a QoS request.
        std::string correlation_id_;

        // Callback signal object.
        SignalType signal_;

        // user public & private ip address.
        std::string public_ip_;
        std::string local_ip_;

    private:
        // SHA-256 encrypt.
        void Encrypt(const unsigned char* message, unsigned int len, unsigned char* result);

        // Get public & local ip.
        bool get_ip_address();

        // Construct Authorization header.
        void AddAuthorizationHeaders(struct curl_slist** headerList);

        // Construct ApplyQoSResourceRequest header, must be called after init curl.
        struct curl_slist* ConstructApplyQoSResourceRequestHeaders();
        // Construct ApplyQoSResourceRequest api body.
        std::string ConstructApplyQoSResourceRequestBody();

        // Construct RemoveQoSResourceRequest header.
        struct curl_slist* ConstructRemoveQoSResourceRequestHeaders();

    private:
        std::string m_Realm;
        std::string m_Username;
        std::string m_Password;
        std::string m_Nonce;

        boost::shared_ptr<boost::thread> m_Thread;
    };
}

#endif