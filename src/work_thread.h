#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

#include "safe_queue.h"

class ThreadCommonData {
private:
    bool shutdown;
    std::mutex shutdown_mutex;
public:
    SafeQueue<std::function<void()>> safe_queue;
    std::condition_variable thread_conditional_variable;
    std::mutex thread_mutex;
    ThreadCommonData();
    void set_shutdown(bool shutdown_);
    bool get_shutdown();
};

class WorkThread : std::thread {
private:
    int thread_id;
    std::shared_ptr<ThreadCommonData> thread_common_data_ptr;
public:
    WorkThread(int thread_id_, std::shared_ptr<ThreadCommonData> thread_common_data_ptr_);
    void operator()();
};

