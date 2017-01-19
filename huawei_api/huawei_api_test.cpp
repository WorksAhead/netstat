#include <string>
#include <iostream>

#include <curl/curl.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <string>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

::std::string base64_encode(const ::std::string &bindata)
{
   using ::std::string;
   using ::std::numeric_limits;

   if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
      throw ::std::length_error("Converting too large a string to base64.");
   }

   const ::std::size_t binlen = bindata.size();
   // Use = signs so the end is properly padded.
   string retval((((binlen + 2) / 3) * 4), '=');
   ::std::size_t outpos = 0;
   int bits_collected = 0;
   unsigned int accumulator = 0;
   const string::const_iterator binend = bindata.end();

   for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
      accumulator = (accumulator << 8) | (*i & 0xffu);
      bits_collected += 8;
      while (bits_collected >= 6) {
         bits_collected -= 6;
         retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
      }
   }
   if (bits_collected > 0) { // Any trailing bits that are missing.
      assert(bits_collected < 6);
      accumulator <<= 6 - bits_collected;
      retval[outpos++] = b64_table[accumulator & 0x3fu];
   }
   assert(outpos >= (retval.size() - 2));
   assert(outpos <= retval.size());
   return retval;
}

::std::string base64_decode(const ::std::string &ascdata)
{
   using ::std::string;
   string retval;
   const string::const_iterator last = ascdata.end();
   int bits_collected = 0;
   unsigned int accumulator = 0;

   for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
      const int c = *i;
      if (::std::isspace(c) || c == '=') {
         // Skip whitespace and padding. Be liberal in what you accept.
         continue;
      }
      if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
         throw ::std::invalid_argument("This contains characters not legal in a base64 encoded string.");
      }
      accumulator = (accumulator << 6) | reverse_table[c];
      bits_collected += 6;
      if (bits_collected >= 8) {
         bits_collected -= 8;
         retval += (char)((accumulator >> bits_collected) & 0xffu);
      }
   }
   return retval;
}

namespace pt = boost::property_tree;
using boost::property_tree::write_json;

using namespace std;

string sha256(const string& str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

char *base64(const char *input, int length)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length);
    memcpy(buff, bptr->data, bptr->length-1);
    buff[bptr->length-1] = 0;

    BIO_free_all(b64);

    return buff;
}

int main(int argc, char* argv[])
{
    std::string nonce = "eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==";
    std::string timestamp = "2013-09-05'T'02:12:21'Z'";
    std::string pwd = "Changyou@123";

    std::string clearPwd = nonce + timestamp + pwd;
    std::string encryptedPwd = sha256(clearPwd);
    std::cout << "sha256 pwd: " << encryptedPwd << ", length: " << encryptedPwd.length() << ", SHA256_DIGEST_LENGTH: " << SHA256_DIGEST_LENGTH << std::endl;
    // std::string result = base64(encryptedPwd.c_str(), encryptedPwd.length());
    std::string result = base64_encode(encryptedPwd);

    std::cout << "clear pwd: " << clearPwd << ", length: " << clearPwd.length() << ", encrypted pwd: " << result << ", length: " << result.length() << std::endl;
    std::string decodstr = base64_decode(result);
    std::cout << "decode: " << decodstr << std::endl;


    return 0;

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
        headers = curl_slist_append(headers, "X-WSSE: UsernameToken Username=\"ChangyouDevice\", PasswordDigest=\"Qd0QnQn0eaAHpOiuk/0QhV+Bzdc=\", Nonce=\"eUZZZXpSczFycXJCNVhCWU1mS3ZScldOYg==\", Timestamp=\"2013-09-05T02:12:21Z\"");
        headers = curl_slist_append(headers, "Expect:");

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
        root.put("ServiceId", "open_qos_3");

        pt::ptree resFeatureProps;

        pt::ptree resFeature;
        resFeature.put("Type", 6);
        resFeature.put("Priority", 15);
        
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
