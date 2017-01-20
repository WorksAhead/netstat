#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <string>
#include <iostream>

#if defined (__linux__) || defined (__FreeBSD__)

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

# include <unistd.h>
# include <errno.h>
# include <sys/wait.h>
# include <sys/stat.h>
# include <fcntl.h>

std::string g_LogFilePath;

void daemonize()
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
            std::cerr << "First fork failed." << std::endl;
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
            std::cerr << "Second fork failed." << std::endl;
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
        std::cerr << "Unable to open /dev/null." << std::endl;
        exit(-1);
    }

    // Send standard output to a log file.
    const char* output = g_LogFilePath.c_str();
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (open(output, flags, mode) < 0)
    {
        std::cerr << "Unable to open output file." << std::endl;
        exit(-1);
    }

    // Also send standard error to the same log file.
    if (dup(1) < 0)
    {
        std::cerr << "Unable to dup output descriptor." << std::endl;
        exit(-1);
    }
}

extern char *optarg;

char Usage[] = "\
Usage: latency_test -i [host] -p [port] [-options]\n\
Common options:\n\
	-d	Running as a daemon process.\n\
";

#endif

class service_pool {
private:
	typedef boost::shared_ptr<boost::asio::io_service> service_ptr;

public:
	explicit service_pool(int size) : next_(0)
	{
		if (size < 2) {
			size = 2;
		}

		services_.resize(size);

		for (service_ptr& p : services_)
		{
			p.reset(new boost::asio::io_service);
		}
	}

	void run()
	{
		boost::thread_group thread_pool;

		for (service_ptr& p : services_)
		{
			thread_pool.create_thread(boost::bind(&service_pool::thread_proc, this, p));
		}

		thread_pool.join_all();
	}

	void stop()
	{
		for (service_ptr& p : services_)
		{
			p->stop();
		}
	}

	boost::asio::io_service& next()
	{
		const int i = next_;

		if (++next_ == services_.size()) {
			next_ = 0;
		}

		return *services_[i];
	}

private:
	void thread_proc(service_ptr p)
	{
		boost::asio::io_service::work work(*p);
		p->run();
	}

private:
	int next_;
	std::vector<service_ptr> services_;
};

class session {
public:
	explicit session(boost::asio::io_service& service) : socket_(service)
	{
	}

	void start()
	{
		boost::system::error_code ec;
		socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);

		if (ec) {
		}

		socket_.async_read_some(boost::asio::buffer(data),
			boost::bind(&session::handle_read, this, boost::placeholders::_1, boost::placeholders::_2));
	}

	boost::asio::ip::tcp::socket& socket()
	{
		return socket_;
	}

private:
	void handle_read(const boost::system::error_code& ec, size_t bytes_transferred)
	{
		if (!ec) {
			socket_.async_write_some(boost::asio::buffer(data, bytes_transferred),
				boost::bind(&session::handle_write, this, boost::placeholders::_1, boost::placeholders::_2));
		}
		else {
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& ec, size_t bytes_transferred)
	{
		if (!ec) {
			socket_.async_read_some(boost::asio::buffer(data),
				boost::bind(&session::handle_read, this, boost::placeholders::_1, boost::placeholders::_2));
		}
		else {
			delete this;
		}
	}

private:
	boost::asio::ip::tcp::socket socket_;
	char data[1024];
};

class server {
public:
	server(const std::string& ip, unsigned short port) :
		sp_(boost::thread::hardware_concurrency()),
		acceptor_(sp_.next(), boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip), port))
	{
	}

	void start_accept()
	{
		session* new_session(new session(sp_.next()));

		acceptor_.async_accept(new_session->socket(),
			boost::bind(&server::handle_accept, this, new_session, boost::placeholders::_1));
	}

	void handle_accept(session* new_session, const boost::system::error_code& ec)
	{
		if (!ec) {
			new_session->start();
		}
		else {
			delete new_session;
		}

		start_accept();
	}

	void run()
	{
		start_accept();

		sp_.run();
	}

private:
	service_pool sp_;
	boost::asio::ip::tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
        const char* ip;
        unsigned short port;

#if defined (__linux__) || defined (__FreeBSD__)
	g_LogFilePath = boost::str(boost::format("./%1%.log") % boost::filesystem::basename(argv[0]));

        int c;
        bool is_daemonize = false;

	if (argc < 5) goto usage;

        while ((c = getopt(argc, argv, "di:p:")) != -1) {
		switch (c) {
		case 'd':
			is_daemonize = true;
			break;
                case 'i':
                        ip = optarg;
                        break;
                case 'p':
                        port = atoi(optarg);
                        break;
                default:
                        goto usage;
                }
        }

	if (is_daemonize)
	{
		std::cout << "Process is running as a daemon." << std::endl;
		daemonize();
	}
#else
	if (argc < 3) {
		std::cout << "latency_server ip port" << std::endl;
		return 0;
	}

        ip = argv[1];
        port = boost::lexical_cast<unsigned short>(argv[2]);
#endif

	try {
		server s(ip, port);
		s.run();
	}
	catch (const boost::system::error_code& ec) {
		std::cout << ec.message() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	catch (...) {
		std::cout << "unknown exception" << std::endl;
	}

#if defined (__linux__) || defined (__FreeBSD__)
usage:
	std::cerr << Usage;
        exit(1);
#endif
}

