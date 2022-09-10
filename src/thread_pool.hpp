#pragma once

#include <future>

#include "work_thread.h"
#include "log.hpp"

class ThreadPool {
private:
    std::shared_ptr<ThreadCommonData> thread_common_data_ptr;
    std::vector<std::thread> thread_vector;
    int thread_number;
public:
    ThreadPool(int thread_number_);
    ~ThreadPool();
    void Run();
    void Shutdown();
    template <typename F, typename... Args>
    auto Submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>;
};

template <typename F, typename... Args>
auto ThreadPool::Submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    Log::GetInstance()->WriteLogDefault(0, "ThreadPool get a function.\n");
    std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

    std::function<void()> warpper_func = [task_ptr]()
    {
        (*task_ptr)();
    };

    thread_common_data_ptr -> safe_queue.push(warpper_func);
    thread_common_data_ptr -> thread_conditional_variable.notify_one();

    return task_ptr -> get_future();
}