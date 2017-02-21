#ifndef HUAWEI_API_SERVER_IO_SERVICE_POOL_H_
#define HUAWEI_API_SERVER_IO_SERVICE_POOL_H_

#include <vector>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace huawei_api_server
{
    // A pool of io_service objects.
    class IOServicePool : private boost::noncopyable
    {
    public:
        // Construct the io_service pool.
        explicit IOServicePool(std::size_t poolSize);

        // Run all io_service objects in the pool.
        void Run();
        // Stop all io_service objects in the pool.
        void Stop();

        // Get an io_service to use.
        boost::asio::io_service& GetIOService();

    private:
        typedef boost::shared_ptr<boost::asio::io_service> IOServicePtr;
        typedef boost::shared_ptr<boost::asio::io_service::work> WorkPtr;

        // The pool of io_services.
        std::vector<IOServicePtr> m_IOServiceVector;
        // The work that keeps the io_services running.
        std::vector<WorkPtr> m_WorkVector;
        // The next io_service to use for a connection.
        std::size_t m_NextIOService;
    };
}

#endif // HUAWEI_API_SERVER_IO_SERVICE_POOL_H_
