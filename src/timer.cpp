#include "timer.h"

std::mutex Timer::lock;
int Timer::epollfd;
std::priority_queue<Timer::expire_sock, std::vector<Timer::expire_sock>, std::greater<Timer::expire_sock> > Timer::sock_queue;
std::unordered_map<int, time_t> Timer::expire_map;

Timer::Timer() {

}

Timer::~Timer() {

}

void Timer::Init(int epollfd_) {
    Log::GetInstance()->WriteLogDefault(1, "[timer] Timer init epollfd:%d...\n", epollfd_);
    epollfd = epollfd_;
} 

void Timer::Run() {
    Log::GetInstance()->WriteLogDefault(1, "[timer] Timer run...\n");
    AddSig(SIGALRM, SigHandler, false);
    alarm(ALARMTIMESLOT);
}

void Timer::SigHandler(int sig) {
    auto t = std::thread([]() {
        Tick();
    });
    t.detach();
    // Tick();
    alarm(ALARMTIMESLOT);
}

void Timer::Tick() {
    std::lock_guard<std::mutex> guard(lock);
    Log::GetInstance()->WriteLogDefault(0, "[timer] Timer tick...\n");
    time_t cur = time(NULL);
    while (sock_queue.size()) {
        auto p = sock_queue.top();
        int sockfd = p.second;
        sock_queue.pop();
        if (p.first + CONNECTTIMESLOT > cur) {
            break;
        }
        if (expire_map.find(sockfd) == expire_map.end() || expire_map[sockfd] != p.first) {
            continue;
        }
        DeleteSock(sockfd);
    }
}

void Timer::AddSock(int sockfd) {
    std::lock_guard<std::mutex> guard(lock);
    Log::GetInstance()->WriteLogDefault(0, "[timer] Add sockfd:%d...\n", sockfd);
    time_t cur = time(NULL);
    expire_map[sockfd] = cur;
    sock_queue.push(expire_sock(cur, sockfd));
}

void Timer::DeleteSock(int sockfd) {
    std::lock_guard<std::mutex> guard(lock);
    Log::GetInstance()->WriteLogDefault(1, "[timer] Delete sockfd:%d...\n", sockfd);
    expire_map.erase(sockfd);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
    HttpConnect::user_count--;
}

void Timer::FlushSock(int sockfd) {
    std::lock_guard<std::mutex> guard(lock);
    Log::GetInstance()->WriteLogDefault(0, "[Timer] Flush sockfd:%d...\n", sockfd);
    time_t cur = time(NULL);
    Timer::expire_map[sockfd] = cur;
    sock_queue.push(expire_sock(cur, sockfd));
}
