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

void log(const char* msg)
{
	std::cout << msg << std::endl;
}

int main()
{
	void* p = latency_client_create();

	if (p)
	{
		latency_client_set_log_callback(p, log);
		latency_client_set_log_file(p, "log.txt");
		latency_client_set_endpoint(p, "localhost", "3000");

		for (int i = 0; i < 10; ++i)
		{
			latency_client_start(p);
			boost::this_thread::sleep_for(boost::chrono::milliseconds(rand() % 5000));
			latency_client_stop(p);
		}

		latency_client_destory(p);
	}
}

