#include <string>
#include <iostream>
#include <curl/curl.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;
using boost::property_tree::write_json;

int main(int argc, char* argv[])
{
    CURLcode res = CURL_LAST;
    static char *huaweiApiUrl = "http://baidu.com/QoSV1/DynamicQoS";

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
        headers = curl_slist_append(headers, "Authorization: WSSE realm=\"QoS\", profile=\"UsernameToken\"");
        headers = curl_slist_append(headers, "X-WSSE: UsernameToken");
        headers = curl_slist_append(headers, "Username=\"ODB\",PasswordDigest=\"Qd0QnQn0eaAHpOiuk/0QhV+Bzdc=\"");
        headers = curl_slist_append(headers, "Nonce=\"eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==\"");
        headers = curl_slist_append(headers, "Timestamp=\"2013-09-05T02:12:21Z\"");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Create a node
        pt::ptree root;
        root.put("Partner_ID", "000001");
        root.put("User_ID", "000001123456789");
        root.put("User_ID", "000001123456789");

        pt::ptree userId;
        userId.put("PublicIP", "x.x.x.x");
        userId.put("IP", "x.x.x.x");
        userId.put("IMSI", "460030123456789");
        userId.put("MSISDN", "+8613810005678");
        root.add_child("UserIdentifier", userId);

        root.put("OTTchargingId", "xxxxxssssssssyyyyyyynnnnnn123");
        root.put("APN", "APNtest");
        root.put("ServiceId", "BufferedStreamingVideo");

        pt::ptree resFeatureProps;

        pt::ptree resFeature;
        resFeature.put("Type", 1);
        resFeature.put("Priority", 1);
        
        pt::ptree flowProps;
        
        pt::ptree flowProp;
        flowProp.put("Direction", 2);
        flowProp.put("SourceIpAddress", "x.x.x.x");
        flowProp.put("DestinationIpAddress", "y.y.y.y");
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
        root.put("CallBackURL", "http://XXXXXXXXXXXXXXXXXXX");


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