#include <string>
#include <boost/thread.hpp>
#include "src/ttcpclient.h"

using namespace ttcp;

void ReportCB(const std::string& result)
{
    std::cout << "Uploading speed is " << result << std::endl;
}

int main(int ac, char* av[])
{
    // TTcpClient client("192.168.216.130", 5001, &ReportCB);
    TTcpClient client("52.199.165.141", 5001, &ReportCB);

    boost::thread t(boost::bind(&TTcpClient::Run, &client));

    client.Start();

    while(1)
    { }

    return 0;
}