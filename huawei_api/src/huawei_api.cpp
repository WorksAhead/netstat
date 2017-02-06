#include "huawei_api.h"

#include <boost/format.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "json.hpp"
#include "sha2.h"
#include "b64.h"

// interface
typedef void(*huawei_api_callback_t)(int result, const char* msg);

void* huawei_api_client_create();
void huawei_api_client_destory(void* instance);

void huawei_api_client_set_callback(void* instance, huawei_api_callback_t callback);

void huawei_api_client_qos_resource_request(void* instance);
// end

using namespace huawei;

using json = nlohmann::json;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time;

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

void
HuaweiAPI::ApplyQoSResourceRequest(const char* huaweiApiUrl)
{
    // Init curl
    curl_global_init(CURL_GLOBAL_ALL);

    CURL* curl = curl_easy_init();
    if (curl)
    {
        CURLcode res = CURL_LAST;

        // Setting URL.
        curl_easy_setopt(curl, CURLOPT_URL, huaweiApiUrl);

        // Setting headers.
        struct curl_slist *headers = ConstructHeaders();
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Create a node
        json boddy = {
            //{"Partner_ID", "000001" },
            //{ "User_ID", "000001123456789" },
            { "UserIdentifier",
            { "PublicIP", "111.206.12.231" },
            { "IP", "10.12.26.27" },
            { "IMSI", "460030123456789" },
            { "MSISDN", "+8613810005678" },
            },
            { "OTTchargingId", "xxxxxssssssssyyyyyyynnnnnn123" },
            { "APN", "APNtest" },
            { "ServiceId", "open_qos_3" },
            { "ResourceFeatureProperties",{
                { "Type", 6 },
                { "Priority", 15 },
                { "FlowProperties",{
                    { "Direction", 2 },
                    { "SourceIpAddress", "10.12.26.27" },
                    { "DestinationIpAddress", "111.206.12.231" },
                    { "SourcePort", 1000 },
                    { "DestinationPort", 2000 },
                    { "Protocol", "UDP" },
                    { "MaximumUpStreamSpeedRate", 1000000 },
                    { "MaximumDownStreamSpeedRate", 4000000 },
                }
                },
                { "MaximumUpStreamSpeedRate", 200000 },
                { "MaximumDownStreamSpeedRate", 400000 },
            }
            },
            { "Duration", 600 },
            { "CallBackURL", "http://www.changyou.com" }
        };
        json body;
        //body["Partner_ID"] = "000001";
        //body["User_ID"] = "000001123456789";
        body["UserIdentifier"] = {
            { "PublicIP", "111.206.12.231" },
            { "IP", "10.12.26.27" },
            { "IMSI", "460030123456789" },
            //{ "MSISDN", "08613810005678" },
        };
        body["OTTchargingId"] = "xxxxxssssssssyyyyyyynnnnnn123";
        body["APN"] = "APNtest";
        body["ServiceId"] = "open_qos_3";
        json flowP;
        flowP["FlowProperties"] = { {
            { "Direction", 2 },
            { "SourceIpAddress", "10.12.26.27" },
            { "DestinationIpAddress", "111.206.12.231" },
            { "SourcePort", 1000 },
            { "DestinationPort", 2000 },
            { "Protocol", "UDP" },
            { "MaximumUpStreamSpeedRate", 1000000 },
            { "MaximumDownStreamSpeedRate", 4000000 },
            }
        };
        body["ResourceFeatureProperties"] = { {
            { "Type", 6 },
            { "Priority", 15 },
            //flowP,
            { "MaximumUpStreamSpeedRate", 200000 },
            { "MaximumDownStreamSpeedRate", 400000 },
            }
        };
        body["ResourceFeatureProperties"][0]["FlowProperties"] = { {
            { "Direction", 2 },
            { "SourceIpAddress", "10.12.26.27" },
            { "DestinationIpAddress", "111.206.12.231" },
            { "SourcePort", 1000 },
            { "DestinationPort", 2000 },
            { "Protocol", "UDP" },
            { "MaximumUpStreamSpeedRate", 1000000 },
            { "MaximumDownStreamSpeedRate", 4000000 },
            }
        };
        body["Duration"] = 600;
        body["CallBackURL"] = "http://www.changyou.com";

        std::cout << body << std::endl;

        std::string ok = body.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, ok.c_str());

        // GO!
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // Release handles.
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}