#include <cstdlib>
#include <unistd.h>
#include <boost/format.hpp>
#include "../../lib/mytimer.h"
#include "workerthread.h"
#include "lock_free_queue.h"
#include "boost_lock_free_queue.h"
//#include "lock_based_queue.h"
#include "lock_based_container.h"
#include "asio_worker_thread.h"

const int inner_loop_size = 1000;


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

struct BenchMarkContext {
    int value;
    int counter;
    int* p;
};
thread_local BenchMarkContext bmc;
std::atomic<int> total_counter;


class BenchmarkTask {
    int seed_;
public:

    BenchmarkTask() {
        seed_ = rand()%64;
    }
    void operator()() {
        for(int i=0; i<inner_loop_size; ++i) {
            //result_type::value = ((result_type::value + seed_) & 255);
            bmc.value = ((bmc.value + seed_) & 255);
        }
        //++result_type::counter;
        ++bmc.counter;
    }
};

struct BenchmarkInitPolicy {
    void init() {
        bmc.value = 1;
        bmc.counter = 0;
    }
};

struct BenchmarkEndPolicy {
    void end() {
        //std::cout << "value = " << bmc.value << std::endl;
        //std::cout << "counter = " << bmc.counter << std::endl;
        total_counter += bmc.counter;
    }
};


class BenchmarkJob {
private:
    int value_;
public:
    BenchmarkJob() : value_(rand()%64) {}
    int value() const {
        return value_;
    }

};

struct BenchmarkJobPOD {
    int value_;
public:
    int value() const {
        return value_;
    }
};


struct BenchmarkResource {
    static std::atomic<int> total_counter;
};
std::atomic<int> BenchmarkResource::total_counter;

class BenchmarkJobPolicy : public JobExecutionPolicyBase {
private:
    int value_;
    int counter_;
    //std::atomic<int>& total_counter_;
public:
    typedef BenchmarkJob job_type;
    BenchmarkJobPolicy() : value_(1), counter_(0) {}//, total_counter_(BenchMarkResource::total_counter) {}
    void execute(job_type job) {
        for(int i=0; i<inner_loop_size; ++i) {
            value_ = ((value_ + job.value()) & 255);
        }
        ++counter_;
    }
    void end() {
        BenchmarkResource::total_counter += counter_;
    }
};

class BenchmarkJobPointerPolicy : public JobExecutionPolicyBase {
private:
    int value_;
    int counter_;
public:
    typedef BenchmarkJob* job_type;
    BenchmarkJobPointerPolicy() : value_(1), counter_(0) {}
    void execute(job_type job) {
        for(int i=0; i<inner_loop_size; ++i) {
            value_ = ((value_ + job->value()) & 255);
        }
        ++counter_;
    }
    void end() {
        BenchmarkResource::total_counter += counter_;
    }
};

class BenchmarkJobPODPolicy : public JobExecutionPolicyBase {
private:
    int value_;
    int counter_;
public:
    typedef BenchmarkJobPOD job_type;
    BenchmarkJobPODPolicy() : value_(1), counter_(0) {}
    void execute(job_type job) {
        for(int i=0; i<1000; ++i) {
            value_ = ((value_ + job.value()) & 255);
        }
        ++counter_;
    }
    void end() {
        BenchmarkResource::total_counter += counter_;
    }
};

template <template<class> class Container>
void benchmark(std::string benchmark_name, MyTimer& timer, const int thread_size, const int loop_size) {

    WorkerThread<Container, BenchmarkJobPolicy> worker_solo;
    timer.start(benchmark_name);
    for(int i=0; i<loop_size; ++i) {
        worker_solo.post(typename BenchmarkJobPolicy::job_type());
    }
    worker_solo.join();
    timer.stop();
    std::cout << "total_counter: " << BenchmarkResource::total_counter << std::endl;

    WorkerThreadGroup<Container, BenchmarkJobPolicy> worker(thread_size);
    timer.start(benchmark_name + " group");
    for(int i=0; i<loop_size; ++i) {
        //if (i < 100) usleep(10000);
        worker.post(typename BenchmarkJobPolicy::job_type());
    }
    worker.join();
    timer.stop();
    std::cout << "total_counter: " << BenchmarkResource::total_counter << std::endl;


    WorkerThreadGroup<lock_based_queue, BenchmarkJobPointerPolicy> worker_p(thread_size);
    timer.start(benchmark_name + " pointer");
    for(int i=0; i<loop_size; ++i) {
        worker_p.post(new BenchmarkJob);
    }
    worker_p.join();
    timer.stop();
    std::cout << "total_counter: " << BenchmarkResource::total_counter << std::endl;

}


int main(int argc, char** argv) {
    //const int loop_size = 1000000;
    const int loop_size = 100000;
    //const int thread_size = std::thread::hardware_concurrency();
    const int thread_size = 2;
    std::cout << "thread_size = " << thread_size << std::endl;

    MyTimer timer;
    benchmark<lock_based_queue>("lock_based_queue", timer, thread_size, loop_size);
    benchmark<lock_based_stack>("lock_based_stack", timer, thread_size, loop_size);
    benchmark<boost_lock_free_queue>("boost_lock_free_queue", timer, thread_size, loop_size);
    benchmark<boost_lock_free_stack>("boost_lock_free_stack", timer, thread_size, loop_size);

/*
    WorkerThreadGroup<lock_based_queue<std::shared_ptr<BenchMarkTask> > > bm_worker_lbspg(thread_size);
    timer.start("lock_based_queue<std::shared_ptr<BenchMarkTask> > group");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lbspg.submit(std::make_shared<BenchMarkTask>());
    }
    bm_worker_lbspg.join();
    timer.stop();
*/
/*
    WorkerThread<lock_free_queue, BenchMarkJobPolicy> bm_worker_lf;
    timer.start("lock_free_queue<BenchMarkTask>");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lf.post(BenchMarkJob());
    }
    bm_worker_lf.join();
    timer.stop();
    std::cout << "total_counter: " << BenchMarkResource::total_counter << std::endl;
*/
/*
    WorkerThreadGroup<lock_free_queue, BenchMarkJobPolicy> bm_worker_lfg(thread_size);
    timer.start("lock_free_queue<BenchMarkTask>");
    for(int i=0; i<loop_size; ++i) {
        bm_worker_lfg.post(BenchMarkJob());
    }
    bm_worker_lfg.join();
    timer.stop();
    std::cout << "total_counter: " << BenchMarkResource::total_counter << std::endl;
*/


    AsioThreadPool<BenchmarkInitPolicy, BenchmarkEndPolicy> thread_pool(thread_size);

    timer.start("AsioThreadPool");
    for(int i=0; i<loop_size; ++i) {
        thread_pool.post(BenchmarkTask());
    }
    thread_pool.join();
    timer.stop();
    std::cout << "total_counter: " << total_counter << std::endl;
    //std::cout << "result.value(): " << result.value() << std::endl;


    timer.showResults();
}
