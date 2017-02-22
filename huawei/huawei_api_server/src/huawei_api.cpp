#include "huawei_api.h"

#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "json.hpp"
#include "sha2.h"
#include "b64.h"

using namespace huawei_api_server;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

using json = nlohmann::json;

// libcurl CURLOPT_WRITEFUNCTION callback.
static size_t RequestCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    json response = json::parse(ptr);

    int resultCode = response.value("ResultCode", 0);
    std::string result_msg = response.value("ResultMessage", "Success");

    HuaweiAPI* instance = (HuaweiAPI*)userdata;

    // Update CorrelationId.
    instance->set_correlation_id(response.value("CorrelationId", "0"));
    // Update result.
    instance->set_error_code(resultCode);
    instance->set_description(result_msg);

    size_t realsize = size * nmemb;
    return realsize;
}

HuaweiAPI::HuaweiAPI(const std::string& realm, const std::string& username, const std::string& password, const std::string& nonce)
    : realm_{realm}
    , username_{username}
    , password_{password}
    , nonce_{nonce}
{
    // Init curl
    curl_global_init(CURL_GLOBAL_ALL);
}

HuaweiAPI::~HuaweiAPI()
{
    curl_global_cleanup();
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
    std::string clearPwd = nonce_ + timestamp + password_;
    Encrypt((unsigned char*)clearPwd.c_str(), (unsigned int)clearPwd.length(), sha256Result);

    // Get base64 result.
    char* base64Result = b64_encode(sha256Result, SHA256_DIGEST_SIZE);

    // Construct wsse.
    std::string wsse = boost::str(boost::format("X-WSSE: UsernameToken Username=\"%1%\", PasswordDigest=\"%2%\", Nonce=\"%3%\", Timestamp=\"%4%\"") % username_ % base64Result % nonce_ % timestamp);

    // Construct author.
    std::string author = boost::str(boost::format("Authorization: WSSE realm=\"%1%\", profile=\"UsernameToken\"") % realm_);

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
HuaweiAPI::ConstructApplyQoSResourceRequestBody(const std::string& local_ip, const std::string& public_ip)
{
    json body;

    json::object_t userId =
    {
        { "PublicIP", public_ip },
        { "IP", local_ip },
    };

    body["UserIdentifier"] = userId;

    body["OTTchargingId"] = "xxxxxssssssssyyyyyyynnnnnn123";
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
HuaweiAPI::ApplyQoSResourceRequest(const std::string& local_ip, const std::string& public_ip)
{
    CURL* curl_handle = curl_easy_init();
    if (curl_handle)
    {
        // Set URL.
        curl_easy_setopt(curl_handle, CURLOPT_URL, kQosResourceRequestUrl);

        // Set headers.
        struct curl_slist *headers = ConstructApplyQoSResourceRequestHeaders();
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // Set body.
        std::string body = ConstructApplyQoSResourceRequestBody(local_ip, public_ip);
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
            error_code_ = (int)res;
            description_ = curl_easy_strerror(res);
        }

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
    }
    else
    {
        error_code_ = -1;
        description_ = "curl_easy_init failed.";
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
HuaweiAPI::RemoveQoSResourceRequest()
{
    CURL* curl_handle = curl_easy_init();
    if (curl_handle)
    {
        // Set URL.
        std::string huaweiApiUrl = boost::str(boost::format("%1%/%2%") % kQosResourceRequestUrl % correlation_id_);
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
            error_code_ = (int)res;
            description_ = curl_easy_strerror(res);
        }
        else
        {
            description_ = "Unknown Error";

            curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &error_code_);

            if (error_code_ == 204)
            {
                description_ = "No Content";
            }
        }

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl_handle);
    }
    else
    {
        error_code_ = -1;
        description_ = "curl_easy_init failed.";
    }
}
