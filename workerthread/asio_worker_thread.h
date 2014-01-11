#ifndef ASIO_WORKER_THREAD_H
#define ASIO_WORKER_THREAD_H



#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>


struct VoidInitPolicy {
    void init() {}
};

struct VoidClosePolicy {
    void close() {}
};


template <typename InitPolicy, typename EndPolicy>
struct Runner : public InitPolicy, EndPolicy {
    void operator()(boost::asio::io_service& io_service) {
        InitPolicy::init();
        io_service.run();
        EndPolicy::end();
    }
};

template <typename InitPolicy = VoidInitPolicy, typename ClosePolicy = VoidClosePolicy>
class AsioThreadPool {
    boost::asio::io_service io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;
    Runner<InitPolicy, ClosePolicy> runner;
public:
    AsioThreadPool(std::size_t size) {
        work_.reset(new boost::asio::io_service::work(io_service_));
        for (std::size_t i = 0; i < size; ++i) {
            //group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
            group_.create_thread(boost::bind<void>(runner, boost::ref(io_service_)));
        }
    }

    ~AsioThreadPool() {
        work_.reset();
        group_.join_all();
    }

    template <class T>
    void post(T task) {
        io_service_.post(task);
    }

    void join() {
        work_.reset();
        group_.join_all();
    }
};


#endif // ASIO_WORKER_THREAD_H
