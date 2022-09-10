#pragma once

#include <mutex>
#include <cstring>
#include <thread>
#include <fstream>
#include <ctime>
#include <iostream>

#include "safe_queue.h"

class Log {
private:
    const static char* level_info[4];
    const static char* default_file_name;
    const static char* dir;
public:
    using log_pair = std::pair<char*, char*>;
    static Log* GetInstance() {
        static Log log;
        return &log;
    }
    
    template<typename... Args>
    void WriteLog(int level, const char *file_name_, const char *format, Args &&...args);
    template<typename... Args>
    void WriteLogDefault(int level, const char *format, Args &&...args);

private:
    Log();
    virtual ~Log();
    SafeQueue<log_pair> queue;
    char* current_file;
    bool shutdown;
    std::ofstream file_stream;
};

template<typename... Args>
void Log::WriteLog(int level, const char *file_name, const char *format, Args &&...args) {
    if (level <= 1) {
        return;
    }
    time_t now = time(0);
    tm *local_tm = localtime(&now);
    char year[6], month[4], day[4];
    
    snprintf(year, 6, "%04d", local_tm->tm_year + 1900);
    snprintf(month, 4, "%02d", local_tm->tm_mon + 1);
    snprintf(day, 4, "%02d", local_tm->tm_mday);
    
    char hour[4], minute[4], second[4];
    
    snprintf(hour, 4, "%02d", local_tm->tm_hour);
    snprintf(minute, 4, "%02d", local_tm->tm_min);
    snprintf(second, 4, "%02d", local_tm->tm_sec);
    
    char file[128], buffer[1024];

    snprintf(file, 128, "%s/%s_%s_%s_%s.txt", dir, year, month, day, file_name);
    snprintf(buffer, 1024, format, args...);

    if (current_file == nullptr || file != current_file) {
        if (current_file != nullptr) {
            file_stream.close();
        }
        file_stream.open(file, std::ios::app);
        current_file = file;
    }
    file_stream << hour << ":" << minute << ":" << second << " ";
    file_stream << level_info[level] << " ";  
    file_stream << buffer;

    file_stream.close();
    current_file = nullptr;
}

template<typename... Args>
void Log::WriteLogDefault(int level, const char *format, Args &&...args) {
    WriteLog(level, default_file_name, format, args...);
}
