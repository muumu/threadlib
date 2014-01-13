#ifndef LOCK_BASED_CONTAINER_H
#define LOCK_BASED_CONTAINER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stack>

template <typename T, typename Container = std::deque<T> >
class StackWrapper : public std::stack<T, Container> {
public:
    T& next() {
        return std::stack<T, Container>::top();
    }
};

template <typename T, typename Container = std::deque<T> >
class QueueWrapper : public std::queue<T, Container> {
public:
    T& next() {
        return std::queue<T, Container>::front();
    }
};


template<typename Container>
class lock_based_container {
public:
    typedef typename Container::value_type value_type;
private:
    mutable std::mutex mut;
    Container container;
    std::condition_variable cond;
    //std::atomic<bool> done_;
    bool done_;
public:
    lock_based_container() : done_(false) {}
    void push(value_type new_value) {
        std::lock_guard<std::mutex> lk(mut);
        container.push(std::move(new_value));
        cond.notify_one();
    }
    bool wait_and_pop(value_type& value) {
        std::unique_lock<std::mutex> lk(mut);
        cond.wait(lk, [this]{return !container.empty() || done_;});
        if (done_) {
            return false;
        }
        value = std::move(container.next());
        container.pop();
        return true;
    }
    bool try_pop(value_type& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (container.empty()) {
            return false;
        }
        value = std::move(container.next());
        container.pop();
        return true;
    }
    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return container.empty();
    }
    bool done() const {
        return done_;
    }
    void set_done() {
        done_ = true;
        cond.notify_all();
    }
    void reset_done() {
        done_ = false;
    }
};

template <typename T>
class lock_based_stack :
    public lock_based_container<StackWrapper<T, std::deque<T> > > {};

template <typename T>
class lock_based_queue :
    public lock_based_container<QueueWrapper<T, std::deque<T> > > {};


#endif // LOCK_BASED_CONTAINER
