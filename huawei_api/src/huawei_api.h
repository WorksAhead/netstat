#ifndef __NETSTAT_HUAWEI_API__
#define __NETSTAT_HUAWEI_API__

#include <string>

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

    private:
        std::string m_Realm;
        std::string m_Username;
        std::string m_Password;
        std::string m_Nonce;
    };
}

#endif