#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <iostream>
#include <thread>
#include <boost/lockfree/queue.hpp>
#include <boost/type_traits.hpp>

template <typename T>
inline void delete_if_needed(T) {}

template <typename T>
inline void delete_if_needed(T* p) {
    delete p;
}

template <typename ReturnType, typename... Args>
void delete_if_needed(ReturnType (*)(Args... args)) {};

template <typename Functor, typename... Args>
void exec_functor(Functor f, Args&... args) {
    f(args...);
};

template <typename Functor, typename... Args>
void exec_functor(Functor* f, Args&... args) {
    (*f)(args...);
};



template <typename Container>
class Worker {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    Worker(Container& container, result_type& result) : container_(container), result_(result) {}
    void operator()() {
        typename Container::value_type task;
        while (true) {
            if (!container_.wait_and_pop(task)) {
                break;
            }
            exec_functor(task, result_);
            delete_if_needed(task);
        }
        while (container_.try_pop(task)) {
            exec_functor(task, result_);
            delete_if_needed(task);
        }
    }
private:
    Container& container_;
    result_type& result_;
};

/*
template <typename Container>
class Worker<Container, true> {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    Worker(Container& container, result_type& result) :
        container_(container), result_(result) {
    }
    void operator()() {
        value_type task = nullptr;
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

template <typename Container>
class Worker<Container, true> {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    Worker(Container& container, result_type& result) :
        container_(container), result_(result) {
    }
    void operator()() {
        typename Container::value_type task;
        while (true) {
            if (!container_.wait_and_pop(task)) {
                break;
            }
            (*task)(result_);
            delete task;
        }
        while (container_.try_pop(task)) {
            (*task)(result_);
            delete task;
        }
    }
private:
    Container& container_;
    result_type& result_; 
};


template <typename Container, bool isPointer = false>
struct set_done {
    void operator()(Container& container, std::size_t thread_size) {
        container.set_done();
    }
};

template <typename Container>
struct set_done<Container, true> {
    void operator ()(Container& container, std::size_t thread_size) {
        for (std::size_t i=0; i<thread_size; ++i) {
            container.push(nullptr);
        }
    }
};
*/

template <typename Container>
class WorkerThread {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    WorkerThread() : thread_(Worker<Container>(container_, result_)) {}
    
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

template <typename Container>
class WorkerThreadGroup {
public:
    typedef typename Container::value_type value_type;
    typedef typename boost::remove_pointer<value_type>::type::result_type result_type;
    WorkerThreadGroup(int thread_size = 1) : results_(thread_size), threads_(thread_size) {
        for(size_t i=0; i<threads_.size(); ++i) {
            threads_[i] = std::thread(Worker<Container>(container_, results_[i]));
        }
    }
    
    void submit(value_type task) {
        container_.push(task);
    }
    void join() {
        container_.set_done();
        for(auto& thread : threads_) {
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
