#include <cstdlib>
#include <unistd.h>
#include <boost/format.hpp>
#include "../../lib/mytimer.h"
#include "workerthread.h"
#include "lock_free_queue.h"
#include "boost_lock_free_queue.h"
#include "lock_based_queue.h"
#include "asio_worker_thread.h"



class PrintTask {
public:
    typedef int result_type;
    PrintTask() : id_(0) {}
    PrintTask(int id) : id_(id) {}
    void operator()(result_type& i) {
        std::cout << "processed " << id_ << std::endl;
    }
    static void show(const result_type& value) {
        
    }
private:
    int id_;
};

class BenchMarkTask {
public:
    class result_type {
        int value_;
    public:
        int counter;
        result_type() : value_(10), counter(0) {}
        int value() const {
            return value_;
        }
        void set(int v) {
            value_ = v;
        }
    };

    BenchMarkTask() {}
    void operator()(result_type& r) {
        r.set((r.value() + rand()%32) & 255);
        ++r.counter;
    }
    static void show(const result_type& r) {
        std::cout << r.value() << std::endl;
        std::cout << "number of processed task: " << r.counter << std::endl;
    }
};



int main(int argc, char** argv) {

    int loop_size = 1000000;

    WorkerThread<lock_based_queue<BenchMarkTask> > bm_worker;
    MyTimer timer;
    timer.start("lock_based_queue<BenchMarkTask>");
    for(int i=0; i<1000000; ++i) {
        bm_worker.submit(BenchMarkTask());
    }
    bm_worker.join();
    std::cout << "counter: " << bm_worker.getResult().counter << std::endl;
    std::cout << "result: " << bm_worker.getResult().value() << std::endl;
    timer.stop();

    WorkerThread<lock_free_queue<BenchMarkTask> > bm_worker_lf;
    timer.start("lock_free_queue<BenchMarkTask>");
    for(int i=0; i<1000000; ++i) {
        bm_worker_lf.submit(BenchMarkTask());
    }
    bm_worker_lf.join();
    timer.stop();


    WorkerThread<boost_lock_free_queue<BenchMarkTask*> > bm_worker_p;
    timer.start("boost_lock_free_queue<BenchMarkTask*>");
    for(int i=0; i<1000000; ++i) {
        bm_worker_p.submit(new BenchMarkTask);
    }
    bm_worker_p.join();
    timer.stop();
/*
    WorkerThread<boost_lock_free_queue<BenchMarkTask> > bm_worker_b;
    timer.start();
    for(int i=0; i<1000000; ++i) {
        bm_worker_b.submit(BenchMarkTask());
    }
    bm_worker_b.join();
    timer.stop();
*/



    boost::asio::io_service io_service;
    boost_asio_thread_pool tp(io_service, 1);
    BenchMarkTask::result_type result;

    timer.start("boost_asio");
    for(int i=0; i<loop_size; ++i) {
        tp.submit(boost::bind<void>(BenchMarkTask(), boost::ref(result)));
    }
    tp.join();
    timer.stop();
    std::cout << "result.counter: " << result.counter << std::endl;
    std::cout << "result.value(): " << result.value() << std::endl;

    timer.showResults();
}
