#include "huawei_api.h"

#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "json.hpp"
#include "sha2.h"
#include "b64.h"

using namespace huawei;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

using json = nlohmann::json;

// interface
// result 0 is success, everything else is wrong.
typedef void(*huawei_api_callback_t)(int result, const char* msg);

void* huawei_api_create(const char* realm, const char* username, const char* password, const char* nonce);
void huawei_api_destory(void* instance);

void huawei_api_set_callback(void* instance, huawei_api_callback_t callback);

void huawei_api_async_apply_qos_resource_request(void* instance, const char* public_ip, const char* private_ip);
void huawei_api_apply_qos_resource_request(void* instance, const char* public_ip, const char* private_ip);

void huawei_api_async_remove_qos_resource_request(void* instance);
void huawei_api_remove_qos_resource_request(void* instance);
// end

// libcurl CURLOPT_WRITEFUNCTION callback.
static size_t RequestCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    json response = json::parse(ptr);

    int resultCode = response.value("ResultCode", 0);
    std::string resultMsg = response.value("ResultMessage", "Success");
    if (resultCode != 0)
    {
        resultMsg = "<error> " + resultMsg;
    }

    HuaweiAPI* instance = (HuaweiAPI*)userdata;
    // Update CorrelationId.
    instance->correlation_id_ = response.value("CorrelationId", "0");
    // Do Callback.
    instance->signal_(resultCode, resultMsg.c_str());

    size_t realsize = size * nmemb;
    return realsize;
}

HuaweiAPI::HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce)
    : m_Realm{realm}
    , m_Username{username}
    , m_Password{password}
    , m_Nonce{nonce}
{
    // Init curl
    curl_global_init(CURL_GLOBAL_ALL);
}

HuaweiAPI::~HuaweiAPI()
{
    curl_global_cleanup();

    signal_.disconnect_all_slots();

    if (m_Thread != nullptr)
    {
        m_Thread->join();
    }
}

boost::signals2::connection
HuaweiAPI::RegisterCallback(const SignalType::slot_type& subscriber)
{
    return signal_.connect(subscriber);
}

void
HuaweiAPI::Encrypt(const unsigned char* message, unsigned int len, unsigned char* result)
{
    sha256_ctx sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, message, len);
    sha256_final(&sha256, result);
}

void
HuaweiAPI::AddAuthorizationHeaders(struct curl_slist** headerList)
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

    *headerList = curl_slist_append(*headerList, author.c_str());
    *headerList = curl_slist_append(*headerList, wsse.c_str());
}

struct curl_slist*
HuaweiAPI::ConstructApplyQoSResourceRequestHeaders()
{
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    // Add author headers.
    AddAuthorizationHeaders(&headers);

    // Set 'Expect' header field to null.
    headers = curl_slist_append(headers, "Expect:");

    return headers;
}

std::string
HuaweiAPI::ConstructApplyQoSResourceRequestBody(const std::string& public_ip, const std::string& private_ip)
{
    json body;

    json::object_t userId =
    {
        { "PublicIP", public_ip },
        { "IP", private_ip },
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
HuaweiAPI::AsyncApplyQoSResourceRequest(const std::string& public_ip, const std::string& private_ip)
{
    if (m_Thread != nullptr)
    {
        m_Thread->join();
    }
    m_Thread.reset(new boost::thread(&HuaweiAPI::ApplyQoSResourceRequest, this, public_ip, private_ip));
}

void
HuaweiAPI::ApplyQoSResourceRequest(const std::string& public_ip, const std::string& private_ip)
{
    CURL* curl_handle = curl_easy_init();
    if (curl_handle)
    {
        // Set URL.
        curl_easy_setopt(curl_handle, CURLOPT_URL, QOS_RESOURCE_REQUEST_URL);

        // Set headers.
        struct curl_slist *headers = ConstructApplyQoSResourceRequestHeaders();
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // Set body.
        std::string body = ConstructApplyQoSResourceRequestBody(public_ip, private_ip);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, body.c_str());

        // Set callback.
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, RequestCallback);
        // Set callback user data.
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)this);

        // Set useragent.
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // GO!
        CURLcode res = curl_easy_perform(curl_handle);
        /* check for errors */
        if (res != CURLE_OK)
        {
            std::string error_msg = "<error> ";
            error_msg += curl_easy_strerror(res);
            signal_(res, error_msg.c_str());
        }

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
    }
    else
    {
        signal_(-1, "<error> curl_easy_init failed.");
    }
}

struct curl_slist*
HuaweiAPI::ConstructRemoveQoSResourceRequestHeaders()
{
    struct curl_slist *headers = NULL;
    // Add author headers.
    AddAuthorizationHeaders(&headers);
    // Set 'Expect' header field to null.
    headers = curl_slist_append(headers, "Expect:");

    return headers;
}

void
HuaweiAPI::AsyncRemoveQoSResourceRequest()
{
    if (m_Thread != nullptr)
    {
        m_Thread->join();
    }
    m_Thread.reset(new boost::thread(&HuaweiAPI::RemoveQoSResourceRequest, this));
}

void
HuaweiAPI::RemoveQoSResourceRequest()
{
    CURL* curl_handle = curl_easy_init();
    if (curl_handle)
    {
        // Set URL.
        std::string huaweiApiUrl = boost::str(boost::format("%1%/%2%") % QOS_RESOURCE_REQUEST_URL % correlation_id_);
        curl_easy_setopt(curl_handle, CURLOPT_URL, huaweiApiUrl.c_str());

        // Set headers.
        struct curl_slist *headers = ConstructRemoveQoSResourceRequestHeaders();
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // Set DELETE request.
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "DELETE");

        // Set useragent.
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // GO!
        CURLcode res = curl_easy_perform(curl_handle);
        /* check for errors */
        if (res != CURLE_OK)
        {
            std::string error_msg = "<error> ";
            error_msg += curl_easy_strerror(res);
            signal_(res, error_msg.c_str());
        }
        else
        {
            long response_code;
            const char* error_msg = "<error> Unknown Error";

            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);

            if (response_code == 204)
            {
                error_msg = "No Content";
            }

            signal_(response_code, error_msg);
        }

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
    }
    else
    {
        signal_(-1, "<error> curl_easy_init failed.");
    }
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

void huawei_api_async_apply_qos_resource_request(void* instance, const char* public_ip, const char* private_ip)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->AsyncApplyQoSResourceRequest(public_ip, private_ip);
    }
}

void huawei_api_apply_qos_resource_request(void* instance, const char* public_ip, const char* private_ip)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->ApplyQoSResourceRequest(public_ip, private_ip);
    }
}

void huawei_api_async_remove_qos_resource_request(void* instance)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->AsyncRemoveQoSResourceRequest();
    }
}

void huawei_api_remove_qos_resource_request(void* instance)
{
    if (instance)
    {
        ((HuaweiAPI*)instance)->RemoveQoSResourceRequest();
    }
}