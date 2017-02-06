#include <iostream>
#include <boost/thread.hpp>
#include "huawei_api.h"

using namespace huawei;

void curl_callback(int result, const char* msg)
{
    std::cout << std::endl << result << ", " << msg << std::endl;
}

int main(int argc, char* argv[])
{
    std::string realm = "ChangyouRealm";
    std::string username = "ChangyouDevice";
    std::string password = "Changyou@123";
    std::string nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";
    
    HuaweiAPI hua(realm, username, password, nonce);
    hua.RegisterCallback(curl_callback);
    hua.ApplyQoSResourceRequest("http://183.207.208.184/services/QoSV1/DynamicQoS");
    hua.ApplyQoSResourceRequest("http://183.207.208.184/services/QoSV1/DynamicQoS");
    hua.ApplyQoSResourceRequest("http://183.207.208.184/services/QoSV1/DynamicQoS");

    boost::this_thread::sleep_for(boost::chrono::seconds(10));

	return 0;
}
