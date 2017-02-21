#include "io_service_pool.h"

#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

using namespace huawei_api_server;

IOServicePool::IOServicePool(std::size_t poolSize)
    : m_NextIOService(0)
{
    if (poolSize == 0)
    {
        throw std::runtime_error("io_service_pool size is 0");
    }

    // Give all the io_services work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i = 0; i < poolSize; ++i)
    {
        IOServicePtr io_service(new boost::asio::io_service);
        WorkPtr work(new boost::asio::io_service::work(*io_service));
        m_IOServiceVector.push_back(io_service);
        m_WorkVector.push_back(work);
    }
}

void IOServicePool::Run()
{
    // Create a pool of threads to run all of the io_services.
    std::vector<boost::shared_ptr<boost::thread> > threads;
    for (std::size_t i = 0; i < m_IOServiceVector.size(); ++i)
    {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
            boost::bind(&boost::asio::io_service::run, m_IOServiceVector[i])));
        threads.push_back(thread);
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i]->join();
}

void IOServicePool::Stop()
{
    // Explicitly stop all io_services.
    for (std::size_t i = 0; i < m_IOServiceVector.size(); ++i)
        m_IOServiceVector[i]->stop();
}

boost::asio::io_service& IOServicePool::GetIOService()
{
    // Use a round-robin scheme to choose the next io_service to use.
    boost::asio::io_service& io_service = *m_IOServiceVector[m_NextIOService];
    ++m_NextIOService;
    if (m_NextIOService == m_IOServiceVector.size())
        m_NextIOService = 0;
    return io_service;
}
