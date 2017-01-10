#include <jni.h>
#include <string>

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
Java_com_cyou_netstat_MainActivity_StartCases(
        JNIEnv* env,
        jobject /* this */,
        int mode,
        bool bSpeedup,
        jstring logPath)
{
    std::string hello = "Start from C++";
    return true;
}

extern "C" void
Java_com_cyou_netstat_MainActivity_StopCases(
        JNIEnv* env,
        jobject /* this */)
{

}
