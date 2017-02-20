#include "thread_base.h"

using namespace huawei_api_client;

ThreadBase::ThreadBase()
{
    thread_ = new boost::thread(&ThreadBase::Run, boost::ref(*this));
}

ThreadBase::~ThreadBase()
{
    thread_->join();
    delete thread_;
}
