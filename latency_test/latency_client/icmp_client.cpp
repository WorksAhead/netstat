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
typedef void (*icmp_client_log_callback_t)(const char*);

void* icmp_client_create();
void icmp_client_destory(void* instance);

void icmp_client_set_log_callback(void* instance, icmp_client_log_callback_t log_callback);
void icmp_client_set_log_file(void* instance, const char* filename);

void icmp_client_set_endpoint(void* instance, const char* host);

void icmp_client_start(void* instance);
void icmp_client_stop(void* instance);
//

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

class ipv4_header {
public:
	ipv4_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

	unsigned char version() const { return (rep_[0] >> 4) & 0xF; }
	unsigned short header_length() const { return (rep_[0] & 0xF) * 4; }
	unsigned char type_of_service() const { return rep_[1]; }
	unsigned short total_length() const { return decode(2, 3); }
	unsigned short identification() const { return decode(4, 5); }
	bool dont_fragment() const { return (rep_[6] & 0x40) != 0; }
	bool more_fragments() const { return (rep_[6] & 0x20) != 0; }
	unsigned short fragment_offset() const { return decode(6, 7) & 0x1FFF; }
	unsigned int time_to_live() const { return rep_[8]; }
	unsigned char protocol() const { return rep_[9]; }
	unsigned short header_checksum() const { return decode(10, 11); }

	boost::asio::ip::address_v4 source_address() const
	{
		boost::asio::ip::address_v4::bytes_type bytes
			= { { rep_[12], rep_[13], rep_[14], rep_[15] } };
		return boost::asio::ip::address_v4(bytes);
	}

	boost::asio::ip::address_v4 destination_address() const
	{
		boost::asio::ip::address_v4::bytes_type bytes
			= { { rep_[16], rep_[17], rep_[18], rep_[19] } };
		return boost::asio::ip::address_v4(bytes);
	}

	friend std::istream& operator>>(std::istream&, ipv4_header&);

private:
	unsigned short decode(int a, int b) const
	{
		return (rep_[a] << 8) + rep_[b];
	}

private:
	unsigned char rep_[60];
};

class icmp_header {
public:
	enum {
		echo_reply = 0, destination_unreachable = 3, source_quench = 4,
		redirect = 5, echo_request = 8, time_exceeded = 11, parameter_problem = 12,
		timestamp_request = 13, timestamp_reply = 14, info_request = 15,
		info_reply = 16, address_request = 17, address_reply = 18
	};

	icmp_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

	unsigned char type() const { return rep_[0]; }
	unsigned char code() const { return rep_[1]; }
	unsigned short checksum() const { return decode(2, 3); }
	unsigned short identifier() const { return decode(4, 5); }
	unsigned short sequence_number() const { return decode(6, 7); }

	void type(unsigned char n) { rep_[0] = n; }
	void code(unsigned char n) { rep_[1] = n; }
	void checksum(unsigned short n) { encode(2, 3, n); }
	void identifier(unsigned short n) { encode(4, 5, n); }
	void sequence_number(unsigned short n) { encode(6, 7, n); }

	friend std::istream& operator>>(std::istream&, icmp_header&);
	friend std::ostream& operator<<(std::ostream&, const icmp_header&);

private:
	unsigned short decode(int a, int b) const
	{
		return (rep_[a] << 8) + rep_[b];
	}

	void encode(int a, int b, unsigned short n)
	{
		rep_[a] = static_cast<unsigned char>(n >> 8);
		rep_[b] = static_cast<unsigned char>(n & 0xFF);
	}

private:
	unsigned char rep_[8];
};

std::istream& operator>>(std::istream& is, ipv4_header& header)
{
	is.read(reinterpret_cast<char*>(header.rep_), 20);

	if (header.version() != 4) {
		is.setstate(std::ios::failbit);
	}

	std::streamsize options_length = header.header_length() - 20;

	if (options_length < 0 || options_length > 40) {
		is.setstate(std::ios::failbit);
	}
	else {
		is.read(reinterpret_cast<char*>(header.rep_) + 20, options_length);
	}

	return is;
}

template <typename Iterator>
void compute_checksum(icmp_header& header, Iterator body_begin, Iterator body_end)
{
	unsigned int sum = (header.type() << 8) + header.code()
		+ header.identifier() + header.sequence_number();

	Iterator body_iter = body_begin;

	while (body_iter != body_end)
	{
		sum += (static_cast<unsigned char>(*body_iter++) << 8);
		if (body_iter != body_end)
			sum += static_cast<unsigned char>(*body_iter++);
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	header.checksum(static_cast<unsigned short>(~sum));
}

std::istream& operator>>(std::istream& is, icmp_header& header)
{
	return is.read(reinterpret_cast<char*>(header.rep_), 8);
}

std::ostream& operator<<(std::ostream& os, const icmp_header& header)
{
	return os.write(reinterpret_cast<const char*>(header.rep_), 8);
}

class icmp_client {
public:
	icmp_client()
	{
	}

	void set_log_callback_function(icmp_client_log_callback_t log_callback)
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

	void set_endpoint(const std::string& host)
	{
		try {
			boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), host.c_str(), "");
			boost::asio::io_service service;
			boost::asio::ip::icmp::resolver::iterator it = boost::asio::ip::icmp::resolver(service).resolve(query);
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

			socket_.reset(new boost::asio::ip::icmp::socket(*service_, boost::asio::ip::icmp::v4()));

			timer_.reset(new boost::asio::deadline_timer(*service_));

			thread_.reset(new boost::thread(boost::bind(&icmp_client::thread_proc, this)));

			start_send();
			start_receive();
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

			timer_.reset();

			socket_.reset();

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
	void start_send()
	{
		std::string body("\"Hello!\" from ping.");

		// Create an ICMP header for an echo request.
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
		echo_request.identifier(get_identifier());
		echo_request.sequence_number(++sequence_number_);
		compute_checksum(echo_request, body.begin(), body.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		os << echo_request << body;

		// Send the request.
		time_sent_ = boost::posix_time::microsec_clock::universal_time();
		socket_->send_to(request_buffer.data(), endpoint_);

		// Wait up to five seconds for a reply.
		num_replies_ = 0;
		timer_->expires_at(time_sent_ + boost::posix_time::seconds(5));
		timer_->async_wait(boost::bind(&icmp_client::handle_timeout, this));
	}

	void handle_timeout()
	{
		if (num_replies_ == 0) {
			log("request timed out");
		}

		// Requests must be sent no less than one second apart.
		timer_->expires_at(time_sent_ + boost::posix_time::seconds(1));
		timer_->async_wait(boost::bind(&icmp_client::start_send, this));
	}

	void start_receive()
	{
		// Discard any data already in the buffer.
		reply_buffer_.consume(reply_buffer_.size());

		// Wait for a reply. We prepare the buffer to receive up to 64KB.
		socket_->async_receive(reply_buffer_.prepare(65536),
			boost::bind(&icmp_client::handle_receive, this, boost::placeholders::_2));
	}

	void handle_receive(std::size_t length)
	{
		// The actual number of bytes received is committed to the buffer so that we
		// can extract it using a std::istream object.
		reply_buffer_.commit(length);

		// Decode the reply packet.
		std::istream is(&reply_buffer_);
		ipv4_header ipv4_hdr;
		icmp_header icmp_hdr;
		is >> ipv4_hdr >> icmp_hdr;

		// We can receive all ICMP packets received by the host, so we need to
		// filter out only the echo replies that match the our identifier and
		// expected sequence number.
		if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == get_identifier()
			&& icmp_hdr.sequence_number() == sequence_number_)
		{
			// If this is the first reply, interrupt the five second timeout.
			if (num_replies_++ == 0) {
				timer_->cancel();
			}

			// Print out some information about the reply packet.
			boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

			log("bytes=%1% time=%2%ms ttl=%3%",
				length - ipv4_hdr.header_length(),
				(now - time_sent_).total_milliseconds(),
				ipv4_hdr.time_to_live());
		}

		start_receive();
	}

	static unsigned short get_identifier()
	{
#if defined(BOOST_ASIO_WINDOWS)
		return static_cast<unsigned short>(::GetCurrentProcessId());
#else
		return static_cast<unsigned short>(::getpid());
#endif
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
	icmp_client_log_callback_t log_callback_;

	boost::shared_ptr<std::fstream> log_file_;

	boost::mutex log_mutex;

	boost::asio::ip::icmp::endpoint endpoint_;

	boost::shared_ptr<boost::asio::io_service> service_;

	boost::shared_ptr<boost::asio::ip::icmp::socket> socket_;

	boost::shared_ptr<boost::asio::deadline_timer> timer_;

	boost::shared_ptr<boost::thread> thread_;

	unsigned short sequence_number_;

	boost::posix_time::ptime time_sent_;

	boost::asio::streambuf reply_buffer_;

	std::size_t num_replies_;
};

void* icmp_client_create()
{
	try {
		return new icmp_client;
	}
	catch (...) {
		return nullptr;
	}
}

void icmp_client_destory(void* instance)
{
	if (instance) {
		delete ((icmp_client*)instance);
	}
}

void icmp_client_set_log_callback(void* instance, icmp_client_log_callback_t log_callback)
{
	if (instance) {
		((icmp_client*)instance)->set_log_callback_function(log_callback);
	}
}

void icmp_client_set_log_file(void* instance, const char* filename)
{
	if (instance) {
		((icmp_client*)instance)->set_log_file(filename);
	}
}

void icmp_client_set_endpoint(void* instance, const char* host)
{
	if (instance) {
		((icmp_client*)instance)->set_endpoint(host);
	}
}

void icmp_client_start(void* instance)
{
	if (instance) {
		((icmp_client*)instance)->start();
	}
}

void icmp_client_stop(void* instance)
{
	if (instance) {
		((icmp_client*)instance)->stop();
	}
}
