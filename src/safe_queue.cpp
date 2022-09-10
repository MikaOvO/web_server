#pragma once

#include "safe_queue.h"

template<typename T>
SafeQueue<T>::SafeQueue() {

}

template<typename T>
SafeQueue<T>::~SafeQueue() {

}

template<typename T>
bool SafeQueue<T>::empty() {
    std::lock_guard<std::mutex> lock(mu);
    return que.empty();
}

template<typename T>
size_t SafeQueue<T>::size() {
    std::lock_guard<std::mutex> lock(mu);
    return que.size();
}

template<typename T>
void SafeQueue<T>::push(T t) {
    std::lock_guard<std::mutex> lock(mu);
    que.push(t);
}

template<typename T>
T SafeQueue<T>::front() {
    std::lock_guard<std::mutex> lock(mu);
    return que.front();
}

template<typename T>
void SafeQueue<T>::pop() {
    std::lock_guard<std::mutex> lock(mu);
    que.pop();
}

template<typename T>
bool SafeQueue<T>::pop_with_check(T &t) {
    std::lock_guard<std::mutex> lock(mu);
    if (que.size() == 0) {
        return false;
    }
    t = que.front();
    que.pop();
    return true;
}

