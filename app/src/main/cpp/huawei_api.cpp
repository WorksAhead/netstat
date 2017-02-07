#include "huawei_api.h"

#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "json.hpp"
#include "sha2.h"
#include "b64.h"

using namespace huawei;

using json = nlohmann::json;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

// interface
// result 0 is success, everything else is wrong.
typedef void(*huawei_api_callback_t)(int result, const char* msg);

void* huawei_api_create(const char* realm, const char* username, const char* password, const char* nonce);
void huawei_api_destory(void* instance);

void huawei_api_set_callback(void* instance, huawei_api_callback_t callback);

void huawei_api_async_apply_qos_resource_request(void* instance, const char* url);
void huawei_api_apply_qos_resource_request(void* instance, const char* url);

void huawei_api_async_remove_qos_resource_request(void* instance, const char* url);
void huawei_api_remove_qos_resource_request(void* instance, const char* url);
// end

HuaweiAPI::HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce)
    : m_Realm{realm}
    , m_Username{username}
    , m_Password{password}
    , m_Nonce{nonce}
{
}

HuaweiAPI::~HuaweiAPI()
{
}

boost::signals2::connection
HuaweiAPI::RegisterCallback(const SignalType::slot_type& subscriber)
{
    return m_Signal.connect(subscriber);
}

void
HuaweiAPI::Encrypt(const unsigned char* message, unsigned int len, unsigned char* result)
{
    sha256_ctx sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, message, len);
    sha256_final(&sha256, result);
}

struct curl_slist*
HuaweiAPI::ConstructHeaders()
{
    // Get timestamp
    time_zone_ptr timeZone{ new posix_time_zone{ "CET+8" } };
    local_date_time currentTime{ second_clock::universal_time(), timeZone };
    ptime localTime = currentTime.local_time();

    int year = localTime.date().year();
    int month = localTime.date().month();
    int day = localTime.date().day();
    int hours = localTime.time_of_day().hours();
    int minutes = localTime.time_of_day().minutes();
    int seconds = localTime.time_of_day().seconds();

    std::string timestamp = boost::str(boost::format("%1$04d-%2$02d-%3$02dT%4$02d:%5$02d:%6$02dZ") % year % month % day % hours % minutes % seconds);

    // Get sha-256 value.
    unsigned char sha256Result[SHA256_DIGEST_SIZE];
    std::string clearPwd = m_Nonce + timestamp + m_Password;
    Encrypt((unsigned char*)clearPwd.c_str(), (unsigned int)clearPwd.length(), sha256Result);

    // Get base64 result.
    char* base64Result = b64_encode(sha256Result, SHA256_DIGEST_SIZE);

    // Construct wsse.
    std::string wsse = boost::str(boost::format("X-WSSE: UsernameToken Username=\"%1%\", PasswordDigest=\"%2%\", Nonce=\"%3%\", Timestamp=\"%4%\"") % m_Username % base64Result % m_Nonce % timestamp);

    // Construct author.
    std::string author = boost::str(boost::format("Authorization: WSSE realm=\"%1%\", profile=\"UsernameToken\"") % m_Realm);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, author.c_str());
    headers = curl_slist_append(headers, wsse.c_str());
    // Set 'Expect' header field to null.
    headers = curl_slist_append(headers, "Expect:");

    return headers;
}

std::string
HuaweiAPI::ConstructQoSResourceRequestBody()
{
    json body;

    json::object_t userId =
    {
        { "PublicIP", "111.206.12.231" },
        { "IP", "10.12.26.27" },
        { "IMSI", "460030123456789" },
    };

    body["UserIdentifier"] = userId;

    body["OTTchargingId"] = "xxxxxssssssssyyyyyyynnnnnn123";
    body["APN"] = "APNtest";
    body["ServiceId"] = "open_qos_3";

    json::array_t resFeatureProps;

    json::array_t flowProps;
    json::object_t flowProperty =
    {
        { "Direction", 2 },
        { "SourceIpAddress", "10.12.26.27" },
        { "DestinationIpAddress", "111.206.12.231" },
        { "SourcePort", 1000 },
        { "DestinationPort", 2000 },
        { "Protocol", "UDP" },
        { "MaximumUpStreamSpeedRate", 1000000 },
        { "MaximumDownStreamSpeedRate", 4000000 }
    };
    flowProps.push_back(flowProperty);

    json::object_t resFeatureProperty =
    {
        { "Type", 6 },
        { "Priority", 15 },
        { "FlowProperties", flowProps },
        { "MaximumUpStreamSpeedRate", 200000 },
        { "MaximumDownStreamSpeedRate", 400000 },
    };
    resFeatureProps.push_back(resFeatureProperty);

    body["ResourceFeatureProperties"] = resFeatureProps;

    body["Duration"] = 600;
    body["CallBackURL"] = "http://www.changyou.com";

    return body.dump();
}

void
HuaweiAPI::AsyncApplyQoSResourceRequest(const char* huaweiApiUrl)
{
    if (m_Thread != nullptr)
    {
        m_Thread->join();
    }
    m_Thread.reset(new boost::thread(&HuaweiAPI::ApplyQoSResourceRequest, this, huaweiApiUrl));
}

void
HuaweiAPI::ApplyQoSResourceRequest(const char* huaweiApiUrl)
{
    // Init curl
    curl_global_init(CURL_GLOBAL_ALL);

    CURL* curl = curl_easy_init();
    if (curl)
    {
        // Setting URL.
        curl_easy_setopt(curl, CURLOPT_URL, huaweiApiUrl);

        // Setting headers.
        struct curl_slist *headers = ConstructHeaders();
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string body = ConstructQoSResourceRequestBody();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());

        // GO!
        CURLcode res = curl_easy_perform(curl);
        m_Signal(res, curl_easy_strerror(res));

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void
HuaweiAPI::AsyncRemoveQoSResourceRequest(const char* huaweiApiUrl)
{

}

void
HuaweiAPI::RemoveQoSResourceRequest(const char* huaweiApiUrl)
{

}

// --------------------------------------------------------------------------

void* huawei_api_create(const char* realm, const char* username, const char* password, const char* nonce)
{
    return new HuaweiAPI(realm, username, password, nonce);
}

void huawei_api_destory(void* instance)
{
    if (instance)
    {
        delete ((HuaweiAPI*)instance);
    }
}

void huawei_api_set_callback(void* instance, huawei_api_callback_t callback)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->RegisterCallback(callback);
    }
}

void huawei_api_async_apply_qos_resource_request(void* instance, const char* url)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->AsyncApplyQoSResourceRequest(url);
    }
}

void huawei_api_apply_qos_resource_request(void* instance, const char* url)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->ApplyQoSResourceRequest(url);
    }
}

void huawei_api_async_remove_qos_resource_request(void* instance, const char* url)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->AsyncRemoveQoSResourceRequest(url);
    }
}

void huawei_api_remove_qos_resource_request(void* instance, const char* url)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->RemoveQoSResourceRequest(url);
    }
}