#include <string>
#include "huawei_api.h"

using namespace huawei;

int main(int argc, char* argv[])
{
    std::string realm = "ChangyouRealm";
    std::string username = "ChangyouDevice";
    std::string password = "Changyou@123";
    std::string nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";
    
    HuaweiAPI hua(realm, username, password, nonce);
    hua.ApplyQoSResourceRequest("http://183.207.208.184/services/QoSV1/DynamicQoS");

	return 0;
}
