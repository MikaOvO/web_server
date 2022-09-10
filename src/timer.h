#pragma once

#include <time.h>
#include <functional>
#include <queue>
#include <vector>
#include <map>
#include <thread>
#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "http.h"
#include "epoll_utils.h"
#include "log.hpp"
#include "utils.h"

class Timer {
public:
    const static long CONNECTTIMESLOT = 3600;
    const static long ALARMTIMESLOT = 5;
    using expire_sock = std::pair<time_t, int>;
public:
    Timer();
    ~Timer();

    static void Init(int epollfd_);
    static void Run();
    static void SigHandler(int sig);
    static void Tick();
    static void AddSock(int sockfd);
    static void DeleteSock(int sockfd);
    static void FlushSock(int sockfd);
public:
    static std::mutex lock;
    static int epollfd;
    static std::priority_queue<expire_sock, std::vector<expire_sock>, std::greater<expire_sock> > sock_queue;
    static std::unordered_map<int, time_t> expire_map;
};