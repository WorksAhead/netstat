#ifndef __NETSTAT_HUAWEI_API__
#define __NETSTAT_HUAWEI_API__

#include <string>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <curl/curl.h>

namespace huawei
{
    class HuaweiAPI
    {
    public:
        typedef boost::signals2::signal<void(int, const char*)> SignalType;

    public:
        HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce);
        ~HuaweiAPI();

        boost::signals2::connection RegisterCallback(const SignalType::slot_type& subscriber);

        void ApplyQoSResourceRequest(const char* huaweiApiUrl);

    private:
        // SHA-256 encrypt.
        void Encrypt(const unsigned char* message, unsigned int len, unsigned char* result);
        // Construct curl http header, must be called after init curl.
        struct curl_slist* ConstructHeaders();
        //
        std::string ConstructQoSResourceRequestBody();

        void ApplyQoSResourceRequestInternal(const char* huaweiApiUrl);

    private:
        std::string m_Realm;
        std::string m_Username;
        std::string m_Password;
        std::string m_Nonce;

        SignalType m_Signal;

        boost::shared_ptr<boost::thread> m_Thread;
    };
}

#endif