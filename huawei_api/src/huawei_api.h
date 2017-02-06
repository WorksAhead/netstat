#ifndef __NETSTAT_HUAWEI_API__
#define __NETSTAT_HUAWEI_API__

#include <string>
#include <curl/curl.h>

namespace huawei
{
    class HuaweiAPI
    {
    public:
        HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce);
        ~HuaweiAPI();

        void ApplyQoSResourceRequest(const char* huaweiApiUrl);

    private:
        // SHA-256 encrypt.
        void Encrypt(const unsigned char* message, unsigned int len, unsigned char* result);

        // Construct curl http header, must be called after init curl.
        struct curl_slist* ConstructHeaders();

    private:
        std::string m_Realm;
        std::string m_Username;
        std::string m_Password;
        std::string m_Nonce;
    };
}

#endif