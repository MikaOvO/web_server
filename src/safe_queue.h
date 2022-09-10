#pragma once

#include <queue>
#include <mutex>
#include <thread>

template<typename T>
class SafeQueue {
private:
    std::queue<T> que;
    std::mutex mu;
public:
    SafeQueue();
    ~SafeQueue();
    bool empty();
    size_t size();
    void push(T t);
    T front();
    void pop();
    bool pop_with_check(T &t);
};

#include "safe_queue.cpp"