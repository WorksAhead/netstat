#include <iostream>
#include <boost/thread.hpp>
#include "huawei_api.h"

using namespace huawei;

// c interface
typedef void(*huawei_api_callback_t)(int result, const char* msg);

void* huawei_api_client_create(const char* realm, const char* username, const char* password, const char* nonce);
void huawei_api_client_destory(void* instance);

void huawei_api_client_set_callback(void* instance, huawei_api_callback_t callback);

void huawei_api_client_async_qos_resource_request(void* instance, const char* url);
void huawei_api_client_qos_resource_request(void* instance, const char* url);

// callback
void curl_callback(int result, const char* msg)
{
    std::cout << std::endl << result << ", " << msg << std::endl;
}

int main(int argc, char* argv[])
{
    const char* realm = "ChangyouRealm";
    const char* username = "ChangyouDevice";
    const char* password = "Changyou@123";
    const char* nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";
    
    void* handle = huawei_api_client_create(realm, username, password, nonce);
    huawei_api_client_set_callback(handle, curl_callback);

    huawei_api_client_async_qos_resource_request(handle, "http://183.207.208.184/services/QoSV1/DynamicQoS");
    huawei_api_client_async_qos_resource_request(handle, "http://183.207.208.184/services/QoSV1/DynamicQoS");
    huawei_api_client_qos_resource_request(handle, "http://183.207.208.184/services/QoSV1/DynamicQoS");

    huawei_api_client_destory(handle);

    boost::this_thread::sleep_for(boost::chrono::seconds(10));

	return 0;
}
