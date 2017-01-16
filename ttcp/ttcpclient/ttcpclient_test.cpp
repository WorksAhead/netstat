#include "ttcpclient.h"
#include <boost/thread.hpp>
#include <boost/program_options.hpp>

using namespace ttcp;
namespace po = boost::program_options;

void ReportCB(const std::string& result)
{
    std::cout << "Uploading speed is " << result << std::endl;
}

void StopTTcpClient(const boost::system::error_code& error, TTcpClient* client)
{
    if (!error)
    {
        client->Stop();
    }
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
        ("port,p", po::value<std::string>(&port)->default_value("5001"), "Destination port, default is 5001.")
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
    boost::asio::io_service io;

    TTcpClient client(address, port, 1000);
    client.RegisterCallback(&ReportCB);

    client.Start();
    boost::thread t(boost::bind(&TTcpClient::Run, &client));

    boost::asio::deadline_timer writeLogTimer(io, boost::posix_time::seconds(10));
    writeLogTimer.async_wait(boost::bind(StopTTcpClient, boost::asio::placeholders::error, &client));

    io.run();
    t.join();

    std::cout << "**** Start again! ****" << std::endl;

    client.Start();
    boost::thread t2(boost::bind(&TTcpClient::Run, &client));

    writeLogTimer.expires_at(writeLogTimer.expires_at() + boost::posix_time::seconds(10));
    writeLogTimer.async_wait(boost::bind(StopTTcpClient, boost::asio::placeholders::error, &client));

    io.reset();
    io.run();
    t2.join();

    return 0;
}