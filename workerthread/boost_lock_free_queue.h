#ifndef BOOST_LOCK_FREE_QUEUE_H
#define BOOST_LOCK_FREE_QUEUE_H


#include <mutex>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>

template <typename T>
class boost_lock_free_queue;

template <typename T>
class boost_lock_free_queue<T*> {
private:
    mutable std::mutex mut;
    boost::lockfree::queue<T*> queue_;
    std::condition_variable cond_;
public:
    typedef T* value_type;
    boost_lock_free_queue() : queue_(128){}
    void push(T* value) {
        while (!queue_.push(value));
        cond_.notify_one();
    }
    void wait_and_pop(T*& value) {
        while(queue_.empty()) {
            std::this_thread::yield();
        }
        queue_.pop(value);
    }
    void set_done() {
        push(nullptr);
    }

};


#endif // BOOST_LOCK_FREE_QUEUE_H
