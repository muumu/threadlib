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
    int seed_;
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

    BenchMarkTask() {
        seed_ = rand()%64;
    }
    void operator()(result_type& r) {
        for(int i=0; i<1000; ++i) {
            r.set((r.value() + seed_) & 255);
        }
        ++r.counter;
    }
    static void show(const result_type& r) {
        std::cout << r.value() << std::endl;
        std::cout << "number of processed task: " << r.counter << std::endl;
    }
};


struct BenchMarkRunner {
    int seed_;
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
    BenchMarkRunner() {
        seed_ = rand()%64;
    }
    void operator()(boost::asio::io_service& io_service) {
        std::cout << "start running on: " << boost::this_thread::get_id() << std::endl;
        io_service.run();
    }
private:
    result_type result_;
};



int main(int argc, char** argv) {

    int loop_size = 1000000;
    const int thread_size = std::thread::hardware_concurrency();

    //BenchMarkTask::result_type result_lb;
    WorkerThread<lock_based_queue<BenchMarkTask> > bm_worker_lb;
    //WorkerThread<lock_based_queue<decltype(boost::bind<void>(BenchMarkTask(), boost::ref(result_lb)))> > bm_worker_lb;
    MyTimer timer;
    timer.start("lock_based_queue<BenchMarkTask>");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lb.submit(BenchMarkTask());
        //bm_worker_lb.submit(boost::bind<void>(BenchMarkTask(), boost::ref(result_lb)));
    }
    bm_worker_lb.join();
    std::cout << "counter: " << bm_worker_lb.getResult().counter << std::endl;
    std::cout << "result: " << bm_worker_lb.getResult().value() << std::endl;
    timer.stop();

    //BenchMarkTask::result_type result_lbg;
    WorkerThreadGroup<lock_based_queue<BenchMarkTask> > bm_worker_lbg(thread_size);
    //WorkerThread<lock_free_queue<decltype(boost::bind<void>(BenchMarkTask(), boost::ref(result_lf)))> > bm_worker_lf;
    timer.start("lock_based_queue<BenchMarkTask> group");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lbg.submit(BenchMarkTask());
        //bm_worker.submit(boost::bind<void>(BenchMarkTask(), boost::ref(result_lf)));
    }
    bm_worker_lbg.join();
    std::cout << "counter: " << bm_worker_lbg.getResult().counter << std::endl;
    std::cout << "result: " << bm_worker_lbg.getResult().value() << std::endl;
    timer.stop();

    WorkerThreadGroup<lock_based_queue<BenchMarkTask*> > bm_worker_lbpg(thread_size);
    timer.start("lock_based_queue<BenchMarkTask*> group");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lbpg.submit(new BenchMarkTask);
    }
    bm_worker_lbpg.join();
    std::cout << "counter: " << bm_worker_lbpg.getResult().counter << std::endl;
    std::cout << "result: " << bm_worker_lbpg.getResult().value() << std::endl;
    timer.stop();



    //BenchMarkTask::result_type result_lf;
    WorkerThread<lock_free_queue<BenchMarkTask> > bm_worker_lf;
    //WorkerThread<lock_free_queue<decltype(boost::bind<void>(BenchMarkTask(), boost::ref(result_lf)))> > bm_worker_lf;
    timer.start("lock_free_queue<BenchMarkTask>");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lf.submit(BenchMarkTask());
        //bm_worker.submit(boost::bind<void>(BenchMarkTask(), boost::ref(result_lf)));
    }
    bm_worker_lf.join();
    timer.stop();


    WorkerThread<boost_lock_free_queue<BenchMarkTask*> > bm_worker_p;
    timer.start("boost_lock_free_queue<BenchMarkTask*>");
    for(int i=0; i<loop_size; ++i) {
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
    boost_asio_thread_pool<BenchMarkRunner> tp(io_service, BenchMarkRunner(), thread_size);
    BenchMarkTask::result_type result;

    timer.start("boost_asio");
    for(int i=0; i<loop_size; ++i) {
        tp.submit(boost::bind<void>(BenchMarkTask(), boost::ref(result)));
    }
    tp.join();
    timer.stop();
    std::cout << "result.counter: " << result.counter << std::endl;
    std::cout << "result.value(): " << result.value() << std::endl;


    BenchMarkTask::result_type res;
    std::vector<std::thread> threads(2);
    timer.start("independent thread");
    for (auto& thread : threads) {
        thread = std::thread(boost::bind<void>(BenchMarkTask(), res));
    }
    for (auto& thread : threads) {
        thread.join();
    }
    timer.stop();
    

    timer.showResults();
}
