#pragma once

#include "thread_pool.hpp"

ThreadPool::ThreadPool(int thread_number_) {
    thread_number = thread_number_;
    thread_common_data_ptr = std::make_shared<ThreadCommonData>();
}

ThreadPool::~ThreadPool() {
    if (! thread_common_data_ptr -> get_shutdown()) {
        Shutdown();
    }
}

void ThreadPool::Run() {
    Log::GetInstance()->WriteLogDefault(1, "[thread_pool] ThreadPool Run...\n");
    for (int i = 0; i < thread_number; ++i) {
        thread_vector.emplace_back(WorkThread(i, thread_common_data_ptr));
    }
}

void ThreadPool::Shutdown() {
    Log::GetInstance()->WriteLogDefault(1, "[thread_pool] ThreadPool Shutdown...\n");
    thread_common_data_ptr -> set_shutdown(true);
    thread_common_data_ptr ->thread_conditional_variable.notify_all();
    for (int i = 0; i < thread_number; ++i) {
        if (thread_vector[i].joinable()) {
            thread_vector[i].join();
        }
    }
}