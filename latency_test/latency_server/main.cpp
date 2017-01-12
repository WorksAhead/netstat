#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <string>
#include <iostream>

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
			std::cout << "read " << bytes_transferred << std::endl;
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
			std::cout << "write " << bytes_transferred << std::endl;
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

int main(int argc, const char* argv[])
{
	if (argc < 3) {
		std::cout << "latency_server ip port" << std::endl;
		return 0;
	}

	try {
		server s(argv[1], boost::lexical_cast<unsigned short>(argv[2]));
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
}

