#include "logger.h"
#include "ttcpserver.h"

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

const char* g_LogFilePath = "./ttcpserver.log";

#if defined (__linux__) || defined (__FreeBSD__)
# include <unistd.h>
# include <errno.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <fcntl.h>

void Daemonize()
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
            TTCP_LOGGER(error) << "First fork failed.";
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
            TTCP_LOGGER(error) << "Second fork failed.";
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
        TTCP_LOGGER(error) << "Unable to open /dev/null.";
        exit(-1);
    }

    // Send standard output to a log file.
    const char* output = g_LogFilePath;
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (open(output, flags, mode) < 0)
    {
        TTCP_LOGGER(error) << "Unable to open output file " << output << ".";
        exit(-1);
    }

    // Also send standard error to the same log file.
    if (dup(1) < 0)
    {
        TTCP_LOGGER(error) << "Unable to dup output descriptor.";
        exit(-1);
    }
}
#endif

using namespace ttcp;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
    // Get command line arguments.
    unsigned short listenPort;
    bool isDaemonize = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Produce help message.")
        ("daemon,d", "Running as a daemon process.")
        ("port,p", po::value<unsigned short>(&listenPort)->default_value(5001), "Listen port, default is 5001.")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << "Usage: ttcpserver [options]" << std::endl;
        std::cout << desc << std::endl;
        std::cout << "Bugs report to <cowcoa@163.com>" << std::endl;
        exit(0);
    }

#if defined (__linux__) || defined (__FreeBSD__)
    if (vm.count("daemon"))
    {
        isDaemonize = true;
    }
#endif

    if (vm.count("port"))
    {
        listenPort = vm["port"].as<unsigned short>();
    }

    // Init log system.
    logging::formatter formatter =
        expr::stream
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d_%H:%M:%S.%f")
        << ": <" << boost::log::trivial::severity << "> "
        << expr::format_named_scope("Scope", keywords::format = "[%f:%l] ")
        << expr::smessage;

    logging::add_common_attributes();
    logging::core::get()->add_global_attribute("Scope", attrs::named_scope());

    if (isDaemonize)
    {
        auto fileSink = logging::add_file_log(
            keywords::file_name = g_LogFilePath
        );
        fileSink->set_formatter(formatter);
        logging::core::get()->add_sink(fileSink);
    }
    else
    {
        auto consoleSink = logging::add_console_log();
        consoleSink->set_formatter(formatter);
        logging::core::get()->add_sink(consoleSink);
    }

#if defined(NDEBUG)
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
#endif

    // Ready to run server.
    boost::asio::io_service io_service;

    // Initialise the server before becoming a daemon. If the process is
    // started from a shell, this means any errors will be reported back to the
    // user.
    TTcpServer server(io_service);

    // Run as daemon or not.
#if defined (__linux__) || defined (__FreeBSD__)
    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(boost::bind(&boost::asio::io_service::stop, &io_service));

    if (isDaemonize)
    {
        // Inform the io_service that we are about to become a daemon. The
        // io_service cleans up any internal resources, such as threads, that may
        // interfere with forking.
        io_service.notify_fork(boost::asio::io_service::fork_prepare);

        Daemonize();

        TTCP_LOGGER(info) << "Process is running as a daemon.";

        // Inform the io_service that we have finished becoming a daemon. The
        // io_service uses this opportunity to create any internal file descriptors
        // that need to be private to the new process.
        io_service.notify_fork(boost::asio::io_service::fork_child);
    }
#endif

    // Server run.
    io_service.run();

    return 0;
}
