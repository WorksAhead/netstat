#include <string>
#include <iostream>

#include <curl/curl.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <string>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

#include "sha2.h"
#include "b64.h"

namespace pt = boost::property_tree;
using boost::property_tree::write_json;

using namespace std;

void sha256_helper(const unsigned char* message, unsigned int len, unsigned char* result)
{
    sha256_ctx sha256;
    sha256_init(&sha256);
    sha256_update(&sha256, message, len);
    sha256_final(&sha256, result);
}

int main(int argc, char* argv[])
{
    std::string nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";
    std::string timestamp = "2013-09-05T02:12:21Z";
    std::string pwd = "Changyou@123";

    unsigned char hash[SHA256_DIGEST_SIZE];

    std::string clearPwd = nonce + timestamp + pwd;
    sha256_helper((unsigned char*)clearPwd.c_str(), clearPwd.length(), hash);
    // std::string result = base64(encryptedPwd.c_str(), encryptedPwd.length());
    char* result = b64_encode(hash, SHA256_DIGEST_SIZE);

    std::cout << "clear pwd: " << clearPwd << ", length: " << clearPwd.length() << ", encrypted pwd: " << result << ", length: " << strlen(result) << std::endl;

    std::string wsse = "X-WSSE: UsernameToken Username=\"ChangyouDevice\", PasswordDigest=";
    wsse += "\"";
    wsse += result;
    wsse += "\", Nonce=\"eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==\", Timestamp=\"2013-09-05T02:12:21Z\"";
    
    CURLcode res = CURL_LAST;
    static char *huaweiApiUrl = "http://183.207.208.184/services/QoSV1/DynamicQoS";

    curl_global_init(CURL_GLOBAL_ALL);

    CURL* curl = curl_easy_init();
    if (curl)
    {
        // Setting URL.
        curl_easy_setopt(curl, CURLOPT_URL, huaweiApiUrl);

        // Setting headers.
        struct curl_slist *headers = NULL;

        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Authorization: WSSE realm=\"ChangyouRealm\", profile=\"UsernameToken\"");
        // headers = curl_slist_append(headers, "X-WSSE: UsernameToken Username=\"ChangyouDevice\", PasswordDigest=\"Qd0QnQn0eaAHpOiuk/0QhV+Bzdc=\", Nonce=\"eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==\", Timestamp=\"2013-09-05T02:12:21Z\"");
        headers = curl_slist_append(headers, wsse.c_str());
        headers = curl_slist_append(headers, "Expect:");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Create a node
        pt::ptree root;
        root.put("Partner_ID", "000001");
        root.put("User_ID", "000001123456789");
        root.put("User_ID", "000001123456789");

        pt::ptree userId;
        userId.put("PublicIP", "111.206.12.231");
        userId.put("IP", "10.12.26.27");
        userId.put("IMSI", "460030123456789");
        userId.put("MSISDN", "+8613810005678");
        root.add_child("UserIdentifier", userId);

        root.put("OTTchargingId", "xxxxxssssssssyyyyyyynnnnnn123");
        root.put("APN", "APNtest");
        root.put("ServiceId", "open_qos_3");

        pt::ptree resFeatureProps;

        pt::ptree resFeature;
        resFeature.put("Type", 6);
        resFeature.put("Priority", 15);
        
        pt::ptree flowProps;
        
        pt::ptree flowProp;
        flowProp.put("Direction", 2);
        flowProp.put("SourceIpAddress", "10.12.26.27");
        flowProp.put("DestinationIpAddress", "111.206.12.231");
        flowProp.put("SourcePort", 1000);
        flowProp.put("DestinationPort", 2000);
        flowProp.put("Protocol", "UDP");
        flowProp.put("MaximumUpStreamSpeedRate", 1000000);
        flowProp.put("MaximumDownStreamSpeedRate", 4000000);

        flowProps.push_back(std::make_pair("", flowProp));

        resFeature.add_child("FlowProperties", flowProps);

        resFeature.put("MinimumUpStreamSpeedRate", 200000);
        resFeature.put("MinimumDownStreamSpeedRate", 400000);

        resFeatureProps.push_back(std::make_pair("", resFeature));

        root.add_child("ResourceFeatureProperties", resFeatureProps);

        root.put("Duration", 600);
        char* urlStr = "http://www.changyou.com&#47test";
        root.put("CallBackURL", urlStr);


        std::ostringstream buf;
        write_json(buf, root, true);

        std::cout << buf.str() << std::endl;
        std::string tt = buf.str();

        const char* ttt = tt.c_str();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, tt.c_str());

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

	return 0;
}
