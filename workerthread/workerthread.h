#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <iostream>
#include <thread>
#include <boost/lockfree/queue.hpp>


enum TypeList {NonPointer, Pointer};

template <typename T>
struct TypeDet {
    typedef T original_type;
    enum {value = TypeList::NonPointer};
};

template <typename T>
struct TypeDet<T*> {
    typedef T original_type;
    enum {value = TypeList::Pointer};
};

template <typename Container, int isPointerType = TypeDet<typename Container::value_type>::value >
class WorkerThread {
public:
    typedef typename Container::value_type::result_type result_type;
    WorkerThread() : thread_(Worker(container_, result_)) {}
    
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
    class Worker {
    public:
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
        typedef typename Container::value_type value_type;
        Container& container_;
        result_type& result_;
    };
    std::thread thread_;
};


template <typename Container>
class WorkerThread <Container, TypeList::Pointer> {
public:
    typedef typename Container::value_type value_type;
    typedef typename TypeDet<value_type>::original_type::result_type result_type;
    WorkerThread() : thread_(Worker(container_, result_)) {}
    
    void submit(value_type task) {
        container_.push(task);
    }
    void join() {
        container_.push(nullptr);
        thread_.join();
    }
    result_type& getResult() {
        return result_;
    }
private:
    Container container_;
    result_type result_;
    class Worker {
    public:
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
    std::thread thread_;
};


#endif // WORKERTHREAD_H
