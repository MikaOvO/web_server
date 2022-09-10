#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <iostream>
#include <strings.h>

#include "http.h"
#include "epoll_utils.h"
#include "timer.h"
#include "log.hpp"
#include "thread_pool.hpp"
#include "utils.h"

class ServerUtils {
public:
    const static int MAX_FD = 65536;
    const static int MAX_EVENTS = 10000;
    const static int THREAD_NUMBER = 8;
public:
    ServerUtils();
    ~ServerUtils();

    void Init(int port_, int OPT_LINGER_, int TRIGMode_);
    void AddUser(int connfd, struct sockaddr_in client_address);
    bool DealClientData();
    void DealWithRead(int sockfd);
    void DealWithWrite(int sockfd);
    void EventListen();
    void EventLoop();

    static void Shutdown(int sig);
private:
    static std::atomic<bool> is_shutdown;

    HttpConnect *users;
    ThreadPool *thread_pool;
    epoll_event *events;

    char *path;

    int port;
    int epollfd;
    int listenfd;
    int OPT_LINGER;
    int TRIGMode;
};

