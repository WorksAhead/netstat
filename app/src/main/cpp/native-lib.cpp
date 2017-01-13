#include <jni.h>
#include <string>
#include <boost/lockfree/queue.hpp>

typedef void (*latency_client_log_callback_t)(const char*);

void* latency_client_create();
void latency_client_destory(void* instance);

void latency_client_set_log_callback(void* instance, latency_client_log_callback_t log_callback);
void latency_client_set_log_file(void* instance, const char* filename);

void latency_client_set_endpoint(void* instance, const char* host, const char* service);

void latency_client_start(void* instance);
void latency_client_stop(void* instance);

void* gLatencyClientInstance = NULL;
boost::lockfree::queue<char*, boost::lockfree::capacity<1024>> gLockfreeQueue;

void NativeLogCallback(const char* message)
{
	char* copyMsg = (char*)malloc(strlen(message));
	strcpy(copyMsg, message);
	gLockfreeQueue.push(copyMsg);
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

	return true;
}

extern "C" bool
Java_com_cyou_netstat_MainActivity_GlobalDestroy(
		JNIEnv* env,
		jobject /* this */)
{
	latency_client_destory(gLatencyClientInstance);
	return true;
}

extern "C" bool
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
	latency_client_set_endpoint(gLatencyClientInstance, "10.1.9.84", "3000");

	// start
	latency_client_start(gLatencyClientInstance);
    return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopDelayTest(
        JNIEnv* env,
        jobject /* this */)
{
	latency_client_stop(gLatencyClientInstance);
}

extern "C" bool
Java_com_cyou_netstat_MainActivity_StartBandwidthTest(
		JNIEnv* env,
		jobject /* this */,
		jstring logPath)
{
	return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopBandwidthTest(
		JNIEnv* env,
		jobject /* this */)
{

}

extern "C" bool
Java_com_cyou_netstat_MainActivity_StartSpeedup(
		JNIEnv* env,
		jobject /* this */)
{
	return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopSpeedup(
		JNIEnv* env,
		jobject /* this */)
{

}
