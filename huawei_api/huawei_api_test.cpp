#include <iostream>
#include <boost/thread.hpp>
#include "huawei_api.h"

using namespace huawei;

// c interface
// result 0 is success, everything else is wrong.
typedef void(*huawei_api_callback_t)(int result, const char* msg);

void* huawei_api_create(const char* realm, const char* username, const char* password, const char* nonce);
void huawei_api_destory(void* instance);

void huawei_api_set_callback(void* instance, huawei_api_callback_t callback);

void huawei_api_async_apply_qos_resource_request(void* instance);
void huawei_api_apply_qos_resource_request(void* instance);

void huawei_api_async_remove_qos_resource_request(void* instance);
void huawei_api_remove_qos_resource_request(void* instance);

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
    
    void* handle = huawei_api_create(realm, username, password, nonce);
    huawei_api_set_callback(handle, curl_callback);

    huawei_api_async_apply_qos_resource_request(handle);
    huawei_api_async_remove_qos_resource_request(handle);

    huawei_api_apply_qos_resource_request(handle);
    huawei_api_remove_qos_resource_request(handle);

    boost::this_thread::sleep_for(boost::chrono::seconds(10));
    
    huawei_api_destory(handle);

	return 0;
}
