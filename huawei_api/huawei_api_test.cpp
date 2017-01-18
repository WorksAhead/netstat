#include <string>
#include <curl/curl.h>

int main(int argc, char* argv[])
{
    std::string username = "zhangxin_by";
    std::string clear_pwd = "cow";

    std::string cyou_username = username + "@cyou-inc.com";

    CURLcode res = CURL_LAST;

    std::string user_pwd = cyou_username + ":" + clear_pwd;
    static char *ldap_url = "LDAP://10.1.0.11/OU=Users,OU=Managed,DC=cyou-inc,DC=com";


    curl_global_init(CURL_GLOBAL_ALL);

    CURL* curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, ldap_url);
        curl_easy_setopt(curl, CURLOPT_USERPWD, user_pwd.c_str());

        res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

	return 0;
}