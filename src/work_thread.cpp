#include "work_thread.h"
#include "log.hpp"

ThreadCommonData::ThreadCommonData(): shutdown(false) {
}

void ThreadCommonData::set_shutdown(bool shutdown_) {
    std::lock_guard<std::mutex> lock(shutdown_mutex);
    shutdown = shutdown_;
}

bool ThreadCommonData::get_shutdown() {
    std::lock_guard<std::mutex> lock(shutdown_mutex);
    return shutdown;
}
WorkThread::WorkThread(int thread_id_, std::shared_ptr<ThreadCommonData> thread_common_data_ptr_) {
    thread_id = thread_id_;
    thread_common_data_ptr = thread_common_data_ptr_;
}

void WorkThread::operator() () {
    std::function<void()> func;
    bool pop_flag;
    while (! thread_common_data_ptr -> get_shutdown()) {
        std::unique_lock<std::mutex> lock(thread_common_data_ptr -> thread_mutex);
        if (thread_common_data_ptr -> safe_queue.empty()) {
            thread_common_data_ptr -> thread_conditional_variable.wait(lock);
        }
        lock.unlock();
        pop_flag = thread_common_data_ptr -> safe_queue.pop_with_check(func);
        if (pop_flag) {
            Log::GetInstance()->WriteLogDefault(0, "[work_thread] Thread %d get a function and run it.\n", thread_id);
            func();
        }
    }
}