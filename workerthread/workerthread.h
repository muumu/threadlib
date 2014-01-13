#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <iostream>
#include <thread>
#include <boost/type_traits.hpp>
#include "../../lib/util.h"


struct JobExecutionPolicyBase {
    void init() {}
    void end() {}
    template <typename T>
    void execute(T job) {
        util::exec_functor(job);
    }
};

struct SpinLockWaitPolicy {
    template <typename Container, typename job_type>
    bool wait_and_pop(Container& container, job_type& job) {
        while (!container.try_pop(job)) {
            if (container.done()) {
                return false;
            }
            usleep(20);
        }
        return true;
    }
};

struct SignalWaitPolicy {
    template <typename Container, typename job_type>
    bool wait_and_pop(Container& container, job_type& job) {
        if (!container.wait_and_pop(job)) {
            return false;
        }
        return true;
    }
};


template <template<class> class Container, typename JobExecutionPolicy,
    typename WaitPolicy = SpinLockWaitPolicy>
class Worker {
public:
    typedef typename JobExecutionPolicy::job_type job_type;

    Worker(Container<job_type>& container) : container_(container) {}

    void operator()() {
        job_exec_policy.init();
        job_type job{};
        while (true) {
            if (!wait_policy.wait_and_pop(container_, job)) {
                break;
            }
            job_exec_policy.execute(job);
            util::delete_if_needed(job);
        }
        while (container_.try_pop(job)) {
            job_exec_policy.execute(job);
            util::delete_if_needed(job);
        }
        job_exec_policy.end();
    }

private:
    Container<job_type>& container_;
    JobExecutionPolicy job_exec_policy;
    WaitPolicy wait_policy;
};


template <template<class> class Container, typename JobExecutionPolicy = JobExecutionPolicyBase>
class WorkerThreadBase {
public:
    typedef Container<typename JobExecutionPolicy::job_type> container_type;
    typedef typename container_type::value_type value_type;
    
    void post(value_type job) {
        container_.push(job);
    }

protected:
    container_type container_;
};


template <template<class> class Container, typename JobExecutionPolicy = JobExecutionPolicyBase>
class WorkerThread : public WorkerThreadBase<Container, JobExecutionPolicy> {
public:
    WorkerThread() : thread_(Worker<Container, JobExecutionPolicy>(this->container_)) {}
    
    void join() {
        this->container_.set_done();
        thread_.join();
    }

private:
    std::thread thread_;
};

template <template<class> class Container, typename JobExecutionPolicy = JobExecutionPolicyBase>
class WorkerThreadGroup : public WorkerThreadBase<Container, JobExecutionPolicy> {
public:
    WorkerThreadGroup(const int thread_size = 1) : threads_(thread_size) {
        for(size_t i=0; i<thread_size; ++i) {
            threads_[i] = std::thread(Worker<Container, JobExecutionPolicy>(this->container_));
        }
    }

    void join() {
        this->container_.set_done();
        for(auto& thread : threads_) {
            thread.join();
        }
    }

private:
    std::vector<std::thread> threads_;
};




#endif // WORKERTHREAD_H
