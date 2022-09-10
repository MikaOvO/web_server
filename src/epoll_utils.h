#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

int SetNonBlocking(int fd);

void Addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

void Removefd(int epollfd, int fd);

void Modfd(int epollfd, int fd, int ev, int TRIGMode);