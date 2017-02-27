#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include "huawei_api_client.h"

using namespace huawei_api_client;
namespace po = boost::program_options;

typedef void(*huawei_api_client_log_callback_t)(int error_code, const char* description);

void* huawei_api_client_create(const char* address, const char* port);
void huawei_api_client_destory(void* instance);

void huawei_api_client_set_log_callback(void* instance, huawei_api_client_log_callback_t log_callback);
void huawei_api_client_set_log_file(void* instance, const char* filename);

void huawei_api_client_start(void* instance);
void huawei_api_client_stop(void* instance);

void log(int error_code, const char* description)
{
    std::cout << description << std::endl;
}

int main(int argc, char* argv[])
{
    // Get command line arguments.
    std::string address;
    std::string port;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message.")
        ("address,i", po::value<std::string>(&address)->default_value("127.0.0.1"), "Destination address, default is 127.0.0.1.")
        ("port,p", po::value<std::string>(&port)->default_value("6001"), "Destination port, default is 5001.")
        ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (po::error& err)
    {
        std::cerr << "ERROR: " << err.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return -1;
    }

    if (vm.count("help"))
    {
        std::cout << "Usage: ttcpclient_test [options]" << std::endl;
        std::cout << desc << std::endl;
        std::cout << "Bugs report to <cowcoa@163.com>" << std::endl;
        return 0;
    }

    address = vm["address"].as<std::string>();
    port = vm["port"].as<std::string>();

    //
    void* p = huawei_api_client_create(address.c_str(), port.c_str());
    if (p)
    {
        huawei_api_client_set_log_callback(p, log);
        huawei_api_client_set_log_file(p, "log.txt");

        for (int i = 0; i < 10; ++i)
        {
            huawei_api_client_start(p);
            boost::this_thread::sleep_for(boost::chrono::seconds(10));
            huawei_api_client_stop(p);
        }

        huawei_api_client_destory(p);
    }

    return 0;
}