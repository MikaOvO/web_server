#include "server_utils.h"
#include "thread_pool.hpp"
 
std::atomic<bool> ServerUtils::is_shutdown = false;

ServerUtils::ServerUtils() {
    is_shutdown = false;
    AddSig(SIGINT, Shutdown, false);
    AddSig(SIGTERM, Shutdown, false);

    users = new HttpConnect[MAX_FD];
    thread_pool = new ThreadPool(THREAD_NUMBER);
    events = new epoll_event[MAX_EVENTS];

    thread_pool->Run();

    path = getcwd(NULL, 256);
    int split;
    for (int i = 0; i < strlen(path); ++i) {
        if (path[i] == '/') {
            split = i;
        }
    }
    path[split] = '\0';
    strcat(path, "/root");
    listenfd = -1;
    epollfd = -1;
}

ServerUtils::~ServerUtils() {
    if (listenfd != -1) {
        close(listenfd);
    }
    if (epollfd != -1) {
        close(epollfd);
    }

    thread_pool->Shutdown();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    delete[] users;
    delete thread_pool;
    delete[] events;
}

void ServerUtils::Shutdown(int sig) {
    Log::GetInstance()->WriteLogDefault(2, "[server_utils] Get sig %d\n", sig);
    is_shutdown = true;
}

void ServerUtils::Init(int port_, int OPT_LINGER_, int TRIGMode_) {
    port = port_;
    OPT_LINGER = OPT_LINGER_;
    TRIGMode = TRIGMode_;
}

void ServerUtils::EventListen() {
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    if (0 == OPT_LINGER) {
        struct linger tmp = {0, 1};
        setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (1 == OPT_LINGER) {
        struct linger tmp = {1, 1};
        setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }
    
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    
    int flag = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(listenfd, 5);
    assert(ret >= 0);
    
    epollfd = epoll_create(5);
    assert(epollfd != -1);
    
    Addfd(epollfd, listenfd, false, TRIGMode);

    Timer::Init(epollfd);
    Timer::Run();
}

void ServerUtils::AddUser(int connfd, struct sockaddr_in client_address) {
    Addfd(epollfd, connfd, true, TRIGMode);
    users[connfd].Init(epollfd, connfd, client_address, path, TRIGMode);
    Timer::AddSock(connfd);
}

bool ServerUtils::DealClientData() {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    if (0 == TRIGMode) {
        int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0) {
            Log::GetInstance()->WriteLogDefault(3, "[server_utils][DealClientData] errno is:%d\n", errno);
            return false;
        }
        if (HttpConnect::user_count >= MAX_FD) {
            Log::GetInstance()->WriteLogDefault(2, "[server_utils] full fds.\n", errno);
            return false;
        }
        Log::GetInstance()->WriteLogDefault(1, "[server_utils] Get conn ip:%s\n", inet_ntoa(client_address.sin_addr));
        AddUser(connfd, client_address);
    } else {
        while (1) {
            int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0) {
                Log::GetInstance()->WriteLogDefault(3, "[server_utils][DealClientData] errno is:%d\n", errno);
                break;
            }
            if (HttpConnect::user_count >= MAX_FD) {
                Log::GetInstance()->WriteLogDefault(2, "[server_utils] full fds.\n", errno);
                break;
            }
            Log::GetInstance()->WriteLogDefault(1, "[server_utils] Get conn ip:%s\n", inet_ntoa(client_address.sin_addr));
            AddUser(connfd, client_address);
        }
        return false;
    }
    return true;
}

void ServerUtils::DealWithRead(int sockfd) {
    if (users[sockfd].ReadOnce()) {
        Log::GetInstance()->WriteLogDefault(0, "[server_utils] Process sockfd %d\n", sockfd);
        thread_pool->Submit([](HttpConnect &user) {
            user.Process();
        }, std::ref(users[sockfd]));
        Timer::FlushSock(sockfd);
    } else {
        Log::GetInstance()->WriteLogDefault(0, "[server_utils] Read fail, sockfd %d close...\n", sockfd);
        Timer::DeleteSock(sockfd);
    }
}

void ServerUtils::DealWithWrite(int sockfd) {
    if (users[sockfd].Write()) {
        Log::GetInstance()->WriteLogDefault(0, "[server_utils] Write sockfd %d success\n", sockfd);
        Timer::FlushSock(sockfd);
    } else {
        Log::GetInstance()->WriteLogDefault(0, "[server_utils] Write fail, sockfd %d close...\n", sockfd);
        Timer::DeleteSock(sockfd);
    }
}

void ServerUtils::EventLoop() {
    while (! is_shutdown) {
        int number = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (is_shutdown) {
            break;
        }
        if (number < 0 && errno != EINTR) {
            Log::GetInstance()->WriteLogDefault(3, "[server_utils] Epoll error\n");
            break;
        }
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            Log::GetInstance()->WriteLogDefault(1, "[server_utils][EventLoop] Called by fd:%d...\n", sockfd);
            if (sockfd == listenfd) {
                Log::GetInstance()->WriteLogDefault(1, "[server_utils] listen client sockfd: %d\n", sockfd);
                bool flag = DealClientData();
            } else if (events[i].events & EPOLLIN) {
                Log::GetInstance()->WriteLogDefault(1, "[server_utils] sock read sockfd: %d\n", sockfd);
                DealWithRead(sockfd);
            } else if (events[i].events & EPOLLOUT) {
                Log::GetInstance()->WriteLogDefault(1, "[server_utils] sock write sockfd: %d\n", sockfd);
                DealWithWrite(sockfd);
            }
        }
    }
}