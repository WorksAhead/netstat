#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "logger.h"
#include "huawei_api_server.h"

using namespace huawei_api_server;
namespace po = boost::program_options;

#if defined (__linux__) || defined (__FreeBSD__)
# include <unistd.h>
# include <errno.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <fcntl.h>

void Daemonize(const char* log_file_path)
{
    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    if (pid_t pid = fork())
    {
        if (pid > 0)
        {
            // We're in the parent process and need to exit.
            //
            // When the exit() function is used, the program terminates without
            // invoking local variables' destructors. Only global variables are
            // destroyed. As the io_service object is a local variable, this means
            // we do not have to call:
            //
            //   io_service.notify_fork(boost::asio::io_service::fork_parent);
            //
            // However, this line should be added before each call to exit() if
            // using a global io_service object. An additional call:
            //
            //   io_service.notify_fork(boost::asio::io_service::fork_prepare);
            //
            // should also precede the second fork().
            exit(0);
        }
        else
        {
            SERVER_LOGGER(error) << "First fork failed.";
            exit(-1);
        }
    }

    // Make the process a new session leader. This detaches it from the
    // terminal.
    setsid();

    // A process inherits its working directory from its parent. This could be
    // on a mounted filesystem, which means that the running daemon would
    // prevent this filesystem from being unmounted. Changing to the root
    // directory avoids this problem.
    // chdir("/");

    // The file mode creation mask is also inherited from the parent process.
    // We don't want to restrict the permissions on files created by the
    // daemon, so the mask is cleared.
    umask(0);

    // A second fork ensures the process cannot acquire a controlling terminal.
    if (pid_t pid = fork())
    {
        if (pid > 0)
        {
            exit(0);
        }
        else
        {
            SERVER_LOGGER(error) << "Second fork failed.";
            exit(-1);
        }
    }

    // Close the standard streams. This decouples the daemon from the terminal
    // that started it.
    close(0);
    close(1);
    close(2);

    // We don't want the daemon to have any standard input.
    if (open("/dev/null", O_RDONLY) < 0)
    {
        SERVER_LOGGER(error) << "Unable to open /dev/null.";
        exit(-1);
    }

    // Send standard output to a log file.
    const char* output = log_file_path;
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (open(output, flags, mode) < 0)
    {
        SERVER_LOGGER(error) << "Unable to open output file " << output << ".";
        exit(-1);
    }

    // Also send standard error to the same log file.
    if (dup(1) < 0)
    {
        SERVER_LOGGER(error) << "Unable to dup output descriptor.";
        exit(-1);
    }
}
#endif

int main(int argc, char* argv[])
{
    std::string log_file_path = boost::str(boost::format("./%1%.log") % boost::filesystem::basename(argv[0]));

    // Get command line arguments.
    std::string address;
    std::string port;
    int thread_num;
    bool is_daemonize = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message.")
        ("daemon,d", "Running as a daemon process.")
        ("address,i", po::value<std::string>(&address)->default_value("0.0.0.0"), "Bind address, default is 0.0.0.0.")
        ("port,p", po::value<std::string>(&port)->default_value("6001"), "Listen port, default is 6001.")
        ("thread,n", po::value<int>(&thread_num)->default_value(boost::thread::hardware_concurrency()), "Worker thread number, default is number of CPUs or cores or hyperthreading units.")
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
        std::cout << "Usage: huawei_api_server [options]" << std::endl;
        std::cout << desc << std::endl;
        std::cout << "Bugs report to <cowcoa@163.com>" << std::endl;
        return 0;
    }

#if defined (__linux__) || defined (__FreeBSD__)
    if (vm.count("daemon"))
    {
        is_daemonize = true;
    }
#endif

    address = vm["address"].as<std::string>();
    port = vm["port"].as<std::string>();
    thread_num = vm["thread"].as<int>();

    // Init log system.
    logging::formatter formatter =
        expr::stream
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d_%H:%M:%S.%f")
        << ": <" << boost::log::trivial::severity << "> "
        << "[" << expr::attr<attrs::current_thread_id::value_type>("ThreadID") << "] "
        ADD_FILE_LINE_ATTRIBUTES
        << expr::smessage;

    logging::add_common_attributes();

    if (is_daemonize)
    {
        auto file_sink = logging::add_file_log(
            keywords::file_name = log_file_path,
            keywords::auto_flush = true
        );
        file_sink->set_formatter(formatter);
        logging::core::get()->add_sink(file_sink);
    }
    else
    {
        auto console_sink = logging::add_console_log();
        console_sink->set_formatter(formatter);
        logging::core::get()->add_sink(console_sink);
    }

#if defined (NDEBUG)
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
#endif

    // Initialise the server before becoming a daemon. If the process is
    // started from a shell, this means any errors will be reported back to the
    // user.
    HuaweiApiServer server(address, port, thread_num);

    // Run as daemon or not.
#if defined (__linux__) || defined (__FreeBSD__)
    if (is_daemonize)
    {
        std::cout << "Process is running as a daemon." << std::endl;
        Daemonize(log_file_path.c_str());
    }
#endif

    // Server run.
    SERVER_LOGGER(info) << "Server is running.";
    server.Run();
    SERVER_LOGGER(info) << "Server is shutting down.";

    return 0;
}
