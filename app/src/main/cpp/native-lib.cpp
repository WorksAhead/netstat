#include "ttcpclient.h"
#include <jni.h>
#include <boost/lockfree/queue.hpp>

/// ip & port to connect
static const char* cLatencyIp = "10.1.9.84";
static const char* cLatencyPort = "3000";
static const char* cTTcpIp = "52.199.165.141";
static const char* cTTcpPort = "5001";

/// latency client
typedef void (*latency_client_log_callback_t)(const char*);
void* latency_client_create();
void latency_client_destory(void* instance);
void latency_client_set_log_callback(void* instance, latency_client_log_callback_t log_callback);
void latency_client_set_log_file(void* instance, const char* filename);
void latency_client_set_endpoint(void* instance, const char* host, const char* service);
void latency_client_start(void* instance);
void latency_client_stop(void* instance);

/// ttcp client
typedef void(*ttcp_client_log_callback_t)(const char*);
void* ttcp_client_create(const char* address, const char* port, uint32_t notifyInterval);
void ttcp_client_destory(void* instance);
void ttcp_client_set_log_callback(void* instance, ttcp_client_log_callback_t log_callback);
void ttcp_client_set_log_file(void* instance, const char* filename);
void ttcp_client_start(void* instance);
void ttcp_client_stop(void* instance);

/// global instances
void* gLatencyClientInstance = NULL;
void* gTTcpClientInstance = NULL;
boost::lockfree::queue<char*, boost::lockfree::capacity<1024>> gLockfreeQueue;

// callback from native code
void NativeLogCallback(const char* message)
{
	char* copyMsg = (char*)malloc(strlen(message));
	strcpy(copyMsg, message);
	if (!gLockfreeQueue.push(copyMsg))
	{
		// > 1024
		free(copyMsg);
	}
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
	{
		return -1;
	}

	return JNI_VERSION_1_6;
}

extern "C" jstring
		Java_com_cyou_netstat_MainActivity_GetNativeLog(
		JNIEnv* env,
		jobject caller)
{
	if (gLockfreeQueue.empty())
	{
		return NULL;
	}

	class QueueOperator
	{
	public:
		void operator() (char* singleMsg)
		{
			NativeString += singleMsg;
			NativeString += "\n";
			free(singleMsg);
		}

		jstring GetJString(JNIEnv* _env)
		{
			jstring jstr = _env->NewStringUTF(NativeString.c_str());
			return jstr;
		}
	private:
		std::string NativeString;
	}queue;

	gLockfreeQueue.consume_all(queue);
	return queue.GetJString(env);
}

extern "C" bool
Java_com_cyou_netstat_MainActivity_GlobalInitialize(
		JNIEnv* env,
		jobject caller)
{
	NativeLogCallback("initializing native libraries...\n");

	// initialize delay library
	gLatencyClientInstance = latency_client_create();
	latency_client_set_log_callback(gLatencyClientInstance, &NativeLogCallback);

	// initialzie ttcp library
	gTTcpClientInstance = ttcp_client_create(cTTcpIp, cTTcpPort, 1000);
	ttcp_client_set_log_callback(gTTcpClientInstance, &NativeLogCallback);

	return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_GlobalDestroy(
		JNIEnv* env,
		jobject /* this */)
{
	latency_client_destory(gLatencyClientInstance);
	ttcp_client_destory(gTTcpClientInstance);
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StartDelayTest(
        JNIEnv* env,
        jobject /* this */,
        jstring logPath)
{
	// set log path
	const char *nativeString = env->GetStringUTFChars(logPath, 0);
	latency_client_set_log_file(gLatencyClientInstance, nativeString);
	env->ReleaseStringUTFChars(logPath, nativeString);

	// set end point
	latency_client_set_endpoint(gLatencyClientInstance, cLatencyIp, cLatencyPort);

	// start
	latency_client_start(gLatencyClientInstance);
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopDelayTest(
        JNIEnv* env,
        jobject /* this */)
{
	latency_client_stop(gLatencyClientInstance);
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StartBandwidthTest(
		JNIEnv* env,
		jobject /* this */,
		jstring logPath)
{
	// set log path
	const char *nativeString = env->GetStringUTFChars(logPath, 0);
	ttcp_client_set_log_file(gTTcpClientInstance, nativeString);
	env->ReleaseStringUTFChars(logPath, nativeString);

	// start
	ttcp_client_start(gTTcpClientInstance);
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopBandwidthTest(
		JNIEnv* env,
		jobject /* this */)
{
	ttcp_client_stop(gTTcpClientInstance);
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StartSpeedup(
		JNIEnv* env,
		jobject /* this */)
{

}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopSpeedup(
		JNIEnv* env,
		jobject /* this */)
{

}
