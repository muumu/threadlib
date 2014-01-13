#ifndef BOOST_LOCK_FREE_QUEUE_H
#define BOOST_LOCK_FREE_QUEUE_H


#include <mutex>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/stack.hpp>

template <typename Container>
class boost_lock_free_container {
private:
    Container container_;
    bool done_;
public:
    typedef typename Container::value_type value_type;
    boost_lock_free_container() : container_(128), done_(false) {}
    void push(value_type value) {
        while (!container_.push(value));
    }
    bool wait_and_pop(value_type& value) {
        while (!container_.pop(value)) {
            if (done_) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }
    bool try_pop(value_type& value) {
        return container_.pop(value);
    }
    bool done() const {
        return done_;
    }
    void set_done() {
        done_ = true;
    }
    void reset_done() {
        done_ = false;
    }
};

template <typename T>
class boost_lock_free_stack :
    public boost_lock_free_container<boost::lockfree::stack<T> > {};

template <typename T>
class boost_lock_free_queue :
    public boost_lock_free_container<boost::lockfree::queue<T> > {};


/*
template <typename T>
class boost_lock_free_queue {
private:
    boost::lockfree::queue<T> queue_;
    bool done_;
public:
    typedef T value_type;
    boost_lock_free_queue() : queue_(128), done_(false) {}
    void push(T value) {
        while (!queue_.push(value));
    }
    bool wait_and_pop(T& value) {
        while (!queue_.pop(value)) {
            if (done_) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }
    bool try_pop(T& value) {
        return queue_.pop(value);
    }
    bool done() const {
        return done_;
    }
    void set_done() {
        done_ = true;
    }

};

template <typename T>
class boost_lock_free_stack {
private:
    boost::lockfree::stack<T> stack_;
    bool done_;
public:
    typedef T value_type;
    boost_lock_free_stack() : stack_(128), done_(false) {}
    void push(T value) {
        while (!stack_.push(value));
    }
    bool wait_and_pop(T& value) {
        while (!stack_.pop(value)) {
            if (done_) {
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }
    bool try_pop(T& value) {
        return stack_.pop(value);
    }
    bool done() {
        return done_;
    }
    void set_done() {
        done_ = true;
    }

};
*/


#endif // BOOST_LOCK_FREE_QUEUE_H
