#include "epoll_utils.h"
#include "log.hpp"

int SetNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    Log::GetInstance()->WriteLogDefault(0, "[epoll_utils] epollfd:%d Addfd:%d\n", epollfd, fd);
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    SetNonBlocking(fd);
}

void Removefd(int epollfd, int fd) {
    Log::GetInstance()->WriteLogDefault(0, "[epoll_utils] epollfd:%d Removefd:%d\n", epollfd, fd);
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void Modfd(int epollfd, int fd, int ev, int TRIGMode) {
    Log::GetInstance()->WriteLogDefault(0, "[epoll_utils] epollfd:%d Modfd:%d\n", epollfd, fd);
    epoll_event event;
    event.data.fd = fd;
    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
