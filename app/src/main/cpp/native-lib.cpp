#include <jni.h>
#include <string>
#include <boost/asio.hpp>
#include <curl.h>

/*
extern "C" jstring
Java_com_cyou_netstat_MainActivity_StartCases(
        JNIEnv* env,
        jobject)
{
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
*/

extern "C" bool
Java_com_cyou_netstat_MainActivity_StartDelayTest(
        JNIEnv* env,
        jobject /* this */,
        jstring logPath)
{
    return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopDelayTest(
        JNIEnv* env,
        jobject /* this */)
{

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
