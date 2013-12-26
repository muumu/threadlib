#ifndef ASIO_WORKER_THREAD_H
#define ASIO_WORKER_THREAD_H



#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

struct RunnerBase {
    void operator()(boost::asio::io_service& io_service) {
        io_service.run();
    }
};


template <typename Runner>
class boost_asio_thread_pool {
    boost::asio::io_service& io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;
public:
    boost_asio_thread_pool(boost::asio::io_service& io_service, Runner runner, std::size_t size)
        : io_service_(io_service)
    {
        work_.reset(new boost::asio::io_service::work(io_service_));

        for (std::size_t i = 0; i < size; ++i) {
            //group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
            group_.create_thread(boost::bind<void>(runner, boost::ref(io_service_)));
        }
    }

    ~boost_asio_thread_pool() {
        work_.reset();
        group_.join_all();
    }

    template <class T>
    void submit(T task) {
        //boost::asio::io_service::strand st(io_service_);
        io_service_.post(task);
        //io_service_.post(st.wrap(std::ref(task)));
    }

    void join() {
        work_.reset();
        group_.join_all();
    }
};



#endif // ASIO_WORKER_THREAD_H
