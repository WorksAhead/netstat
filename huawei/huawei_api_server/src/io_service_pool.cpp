#include "io_service_pool.h"

#include <stdexcept>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

using namespace huawei_api_server;

IoServicePool::IoServicePool(std::size_t pool_size)
    : next_io_service_(0) {
    if (pool_size == 0) {
        throw std::runtime_error("io_service_pool size is 0");
    }

    // Give all the io_services work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i = 0; i < pool_size; ++i) {
        IOServicePtr io_service(new boost::asio::io_service);
        WorkPtr work(new boost::asio::io_service::work(*io_service));
        io_service_vector_.push_back(io_service);
        work_vector_.push_back(work);
    }
}

void IoServicePool::Run() {
    // Create a pool of threads to run all of the io_services.
    std::vector<boost::shared_ptr<boost::thread> > threads;
    for (std::size_t i = 0; i < io_service_vector_.size(); ++i) {
        boost::shared_ptr<boost::thread> thread(new boost::thread(
            boost::bind(&boost::asio::io_service::run, io_service_vector_[i])));
        threads.push_back(thread);
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i]->join();
}

void IoServicePool::Stop() {
    // Explicitly stop all io_services.
    for (std::size_t i = 0; i < io_service_vector_.size(); ++i)
        io_service_vector_[i]->stop();
}

boost::asio::io_service& IoServicePool::GetIOService() {
    // Use a round-robin scheme to choose the next io_service to use.
    boost::asio::io_service& io_service = *io_service_vector_[next_io_service_];
    ++next_io_service_;
    if (next_io_service_ == io_service_vector_.size())
        next_io_service_ = 0;
    return io_service;
}
