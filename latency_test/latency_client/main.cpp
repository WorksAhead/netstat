#include <boost/thread.hpp>

#include <iostream>

typedef void(*latency_client_log_callback_t)(const char*);

void* latency_client_create();
void latency_client_destory(void* instance);

void latency_client_set_log_callback(void* instance, latency_client_log_callback_t log_callback);
void latency_client_set_log_file(void* instance, const char* filename);

void latency_client_set_endpoint(void* instance, const char* host, const char* service);

void latency_client_start(void* instance);
void latency_client_stop(void* instance);

typedef void(*icmp_client_log_callback_t)(const char*);

void* icmp_client_create();
void icmp_client_destory(void* instance);

void icmp_client_set_log_callback(void* instance, icmp_client_log_callback_t log_callback);
void icmp_client_set_log_file(void* instance, const char* filename);

void icmp_client_set_endpoint(void* instance, const char* host);

void icmp_client_start(void* instance);
void icmp_client_stop(void* instance);

void log(const char* msg)
{
	static boost::mutex sync;
	boost::mutex::scoped_lock lock(sync);
	std::cout << msg << std::endl;
}

int main()
{
	void* p = latency_client_create();
	void* p2 = icmp_client_create();

	if (p)
	{
		latency_client_set_log_callback(p, log);
		latency_client_set_log_file(p, "log_latency.txt");
		latency_client_set_endpoint(p, "10.1.9.84", "3000");
	}

	if (p2)
	{
		icmp_client_set_log_callback(p2, log);
		icmp_client_set_log_file(p2, "log_ping.txt");
		icmp_client_set_endpoint(p2, "10.1.9.84");
	}

	for (int i = 0; i < 10; ++i)
	{
		if (p) {
			latency_client_start(p);
		}

		if (p2) {
			icmp_client_start(p2);
		}

		boost::this_thread::sleep_for(boost::chrono::seconds(30));

		if (p) {
			latency_client_stop(p);
		}

		if (p2) {
			icmp_client_stop(p2);
		}
	}

	if (p)
	{
		latency_client_destory(p);
	}

	if (p2)
	{
		icmp_client_destory(p2);
	}
}

