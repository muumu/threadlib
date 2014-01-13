#ifndef ASIO_WORKER_THREAD_H
#define ASIO_WORKER_THREAD_H



#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>


struct VoidInitPolicy {
    void init() {}
};

struct VoidEndPolicy {
    void end() {}
};


template <typename InitPolicy, typename EndPolicy>
struct Runner {
    void operator()(boost::asio::io_service& io_service) {
        init_policy.init();
        io_service.run();
        end_policy.end();
    }
private:
    InitPolicy init_policy;
    EndPolicy end_policy;
};

template <typename InitPolicy = VoidInitPolicy, typename EndPolicy = VoidEndPolicy>
class AsioThreadPool {
    boost::asio::io_service io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;
public:
    AsioThreadPool(std::size_t size) :
        work_(boost::make_shared<boost::asio::io_service::work>(io_service_))
    {
        for (std::size_t i = 0; i < size; ++i) {
            group_.create_thread(boost::bind<void>(Runner<InitPolicy, EndPolicy>(), boost::ref(io_service_)));
        }
    }

    ~AsioThreadPool() {
        work_.reset();
        group_.join_all();
    }

    template <class T>
    void post(T job) {
        io_service_.post(job);
    }

    void join() {
        work_.reset();
        group_.join_all();
    }
};


#endif // ASIO_WORKER_THREAD_H
