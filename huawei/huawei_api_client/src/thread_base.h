#ifndef HUAWEI_API_CLIENT_THREAD_BASE_H_
#define HUAWEI_API_CLIENT_THREAD_BASE_H_

#include <boost/thread.hpp>

namespace huawei_api_client
{
    class ThreadBase
    {
    public:
        ThreadBase();
        virtual ~ThreadBase();

        virtual void Run() = 0;

    protected:
        boost::mutex mutex_;
        boost::thread* thread_;
    };
}

#endif // HUAWEI_API_CLIENT_THREAD_BASE_H_