#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <iostream>
#include <thread>
#include <boost/lockfree/queue.hpp>
#include <boost/type_traits.hpp>



template <typename Container, bool isPointerType = boost::is_pointer<typename Container::value_type>::value>
class Worker {
public:
    typedef typename Container::value_type value_type;
    typedef typename value_type::result_type result_type;
    Worker(Container& container, result_type& result) : container_(container), result_(result) {}
    void operator()() {
        typename Container::value_type task;
        while (true) {
            if (!container_.wait_and_pop(task)) {
                break;
            }
            task(result_);
        }
        while (container_.try_pop(task)) {
            task(result_);
        }
    }
private:
    Container& container_;
    result_type& result_;
};


template <typename Container>
class Worker<Container, true> {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    Worker(Container& container, result_type& result) :
        container_(container), result_(result) {
    }
    void operator()() {
        value_type task;
        while (true) {
            container_.wait_and_pop(task);
            if (task == nullptr) {
                break;
            }
            (*task)(result_);
            delete task;
        }
    }
private:
    Container& container_;
    result_type& result_; 
};



template <typename Container, int isPointer = boost::is_pointer<typename Container::value_type>::value >
class WorkerThread {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    WorkerThread() : thread_(Worker<Container, isPointer>(container_, result_)) {}
    
    void submit(typename Container::value_type task) {
        container_.push(task);
    }
    void join() {
        container_.set_done();
        thread_.join();
    }
    result_type& getResult() {
        return result_;
    }

private:
    Container container_;
    result_type result_;
    std::thread thread_;
};

template <typename Container, int isPointer = boost::is_pointer<typename Container::value_type>::value >
class WorkerThreadGroup {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    WorkerThreadGroup() : results_(4), threads_(4) {
        for(size_t i=0; i<threads_.size(); ++i) {
            threads_[i] = std::thread(Worker<Container, isPointer>(container_, results_[i]));
        }
    }
    
    void submit(typename Container::value_type task) {
        container_.push(task);
    }
    void join() {
        for(auto& thread : threads_) {
            container_.set_done();
            thread.join();
        }
    }
    result_type& getResult() {
        return results_[0];
    }

private:
    Container container_;
    std::vector<result_type> results_;
    std::vector<std::thread> threads_;
};




#endif // WORKERTHREAD_H
