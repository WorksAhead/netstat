#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/random.hpp>

#include <fstream>
#include <string>

#include <stdint.h>

// interface
typedef void (*latency_client_log_callback_t)(const char*);

void* latency_client_create();
void latency_client_destory(void* instance);

void latency_client_set_log_callback(void* instance, latency_client_log_callback_t log_callback);
void latency_client_set_log_file(void* instance, const char* filename);

void latency_client_set_endpoint(void* instance, const char* host, const char* service);

void latency_client_start(void* instance);
void latency_client_stop(void* instance);
// end

namespace Internals {

	inline void bindArgs(int argN, boost::format&)
	{
	}

	template<typename Arg1, typename... Args>
	inline void bindArgs(int argN, boost::format& f, const Arg1& arg1, Args... args)
	{
		f.bind_arg(argN, arg1);
		bindArgs(argN + 1, f, args...);
	}

}

template<typename... Args>
inline void bindArgs(boost::format& f, Args... args)
{
	Internals::bindArgs(1, f, args...);
}

class latency_client {
private:
	enum {
		data_size = 32,
	};

	typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

public:
	latency_client() : log_callback_(nullptr)
	{
	}

	void set_log_callback_function(latency_client_log_callback_t log_callback)
	{
		log_callback_ = log_callback;
	}

	void set_log_file(const std::string& path)
	{
		try {
			log_file_.reset(new std::fstream(path.c_str(), std::ios::out));

			if (!log_file_->is_open()) {
				log_file_.reset();
				log("failed to open log file '%1%'", path);
			}
		}
		catch (std::exception& e) {
			log("set log exception %1%", e.what());
		}
		catch (...) {
			log("set log exception");
		}
	}

	void set_endpoint(const std::string& host, const std::string& service)
	{
		try {
			boost::asio::ip::tcp::resolver::query query(host, service);
			boost::asio::io_service service;
			boost::asio::ip::tcp::resolver::iterator it = boost::asio::ip::tcp::resolver(service).resolve(query);
			endpoint_ = *it;
		}
		catch (boost::system::error_code& ec) {
			log("set endpoint exception %1%", ec.message());
		}
		catch (std::exception& e) {
			log("set endpoint exception %1%", e.what());
		}
		catch (...) {
			log("set endpoint exception");
		}
	}

	void start()
	{
		try {
			log("start");

			service_.reset(new boost::asio::io_service);
			connect_timer_.reset(new boost::asio::deadline_timer(*service_));
			write_timer_.reset(new boost::asio::deadline_timer(*service_));

			thread_.reset(new boost::thread(boost::bind(&latency_client::thread_proc, this)));

			start_connect();
		}
		catch (std::exception& e) {
			log("start exception %1%", e.what());
		}
		catch (...) {
			log("start exception");
		}
	}

	void stop()
	{
		try {
			log("stop");

			service_->stop();

			if (thread_) {
				thread_->join();
				thread_.reset();
			}

			write_timer_.reset();
			connect_timer_.reset();
			service_.reset();
		}
		catch (std::exception& e) {
			log("stop exception %1%", e.what());
		}
		catch (...) {
			log("stop exception");
		}
	}

private:
	void start_connect()
	{
		socket_ptr socket(new boost::asio::ip::tcp::socket(*service_));

		socket->async_connect(endpoint_,
			boost::bind(&latency_client::handle_connect, this, socket, boost::placeholders::_1));
	}

	void start_deferred_connect()
	{
		connect_timer_->expires_from_now(boost::posix_time::seconds(1));
		connect_timer_->async_wait(boost::bind(&latency_client::handle_deferred_connect, this, boost::placeholders::_1));
	}

	void start_write(socket_ptr socket)
	{
		char* p = outbound_data_;

		for (int i = 0; i < data_size / sizeof(uint64_t); ++i) {
			*(uint64_t*)p = rng_();
			p += sizeof(uint64_t);
		}

		boost::asio::async_write(*socket, boost::asio::buffer(outbound_data_),
			boost::bind(&latency_client::handle_write, this, socket, boost::placeholders::_1, boost::placeholders::_2));
	}

	void start_deferred_write(socket_ptr socket, uint64_t count)
	{
		write_timer_->expires_from_now(boost::posix_time::milliseconds(count));
		write_timer_->async_wait(boost::bind(&latency_client::handle_deferred_write, this, socket, boost::placeholders::_1));
	}

	void start_read(socket_ptr socket)
	{
		boost::asio::async_read(*socket, boost::asio::buffer(inbound_data_),
			boost::bind(&latency_client::handle_read, this, socket, boost::placeholders::_1, boost::placeholders::_2));
	}

	void handle_connect(socket_ptr socket, const boost::system::error_code& ec)
	{
		if (ec)
		{
			log("connect error %1%", ec.message());

			start_deferred_connect();
		}
		else
		{
			log("connected");

			boost::system::error_code error_code;
			socket->set_option(boost::asio::ip::tcp::no_delay(true), error_code);

			if (error_code) {
				log("failed to disable nagle");
			}

			start_write(socket);
		}
	}

	void handle_deferred_connect(const boost::system::error_code& ec)
	{
		start_connect();
	}

	void handle_write(socket_ptr socket, const boost::system::error_code& ec, size_t bytes_transferred)
	{
		if (ec)
		{
			log("write error %1%", ec.message());

			start_deferred_connect();
		}
		else
		{
			last_write_time_ = boost::chrono::steady_clock::now();

			start_read(socket);
		}
	}

	void handle_deferred_write(socket_ptr socket, const boost::system::error_code& ec)
	{
		start_write(socket);
	}

	void handle_read(socket_ptr socket, const boost::system::error_code& ec, size_t bytes_transferred)
	{
		if (ec)
		{
			log("read error %1%", ec.message());

			start_deferred_connect();
		}
		else if (memcmp(inbound_data_, outbound_data_, data_size) != 0)
		{
			log("data error");

			start_deferred_connect();
		}
		else
		{
			boost::chrono::steady_clock::duration d = boost::chrono::steady_clock::now() - last_write_time_;
			boost::chrono::milliseconds ms = boost::chrono::duration_cast<boost::chrono::milliseconds>(d);

			log("bytes=%1% time=%2%ms", bytes_transferred, ms.count());

			if (ms.count() > 1000) {
				start_write(socket);
			}
			else {
				start_deferred_write(socket, 1000 - ms.count());
			}
		}
	}

	void thread_proc()
	{
		try {
			boost::asio::io_service::work work(*service_);
			service_->run();
		}
		catch (boost::system::error_code& ec) {
			log("thread exception %1%", ec.message());
		}
		catch (std::exception& e) {
			log("thread exception %1%", e.what());
		}
		catch (...) {
			log("thread exception");
		}
	}

	template<typename... Args>
	void log(const char* s, Args... args)
	{
		try {
			boost::format f(s);
			bindArgs(f, args...);
			log(f.str().c_str());
		}
		catch (...) {
		}
	}

	void log(const char* s)
	{
		try {
			std::string now = boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());

			boost::mutex::scoped_lock lock(log_mutex);

			if (log_file_) {
				*log_file_ << "[" << now << "] " << s << std::endl;
				log_file_->flush();
			}

			if (log_callback_) {
				log_callback_(s);
			}
		}
		catch (...) {
		}
	}

private:
	latency_client_log_callback_t log_callback_;

	boost::shared_ptr<std::fstream> log_file_;

	boost::mutex log_mutex;

	boost::asio::ip::tcp::endpoint endpoint_;

	boost::shared_ptr<boost::asio::io_service> service_;

	boost::shared_ptr<boost::asio::deadline_timer> connect_timer_;

	boost::shared_ptr<boost::asio::deadline_timer> write_timer_;

	boost::shared_ptr<boost::thread> thread_;

	boost::random::mt19937_64 rng_;

	boost::chrono::steady_clock::time_point last_write_time_;

	char outbound_data_[data_size];
	char inbound_data_[data_size];
};

void* latency_client_create()
{
	try {
		return new latency_client;
	}
	catch (...) {
		return nullptr;
	}
}

void latency_client_destory(void* instance)
{
	if (instance) {
		delete ((latency_client*)instance);
	}
}

void latency_client_set_log_callback(void* instance, latency_client_log_callback_t log_callback)
{
	if (instance) {
		((latency_client*)instance)->set_log_callback_function(log_callback);
	}
}

void latency_client_set_log_file(void* instance, const char* filename)
{
	if (instance) {
		((latency_client*)instance)->set_log_file(filename);
	}
}

void latency_client_set_endpoint(void* instance, const char* host, const char* service)
{
	if (instance) {
		((latency_client*)instance)->set_endpoint(host, service);
	}
}

void latency_client_start(void* instance)
{
	if (instance) {
		((latency_client*)instance)->start();
	}
}

void latency_client_stop(void* instance)
{
	if (instance) {
		((latency_client*)instance)->stop();
	}
}

