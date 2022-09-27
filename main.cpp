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

#include "server_utils.h"

int main() {
    ServerUtils server;
    
    server.Init(8088, 0, 1);

    server.EventListen();

    server.EventLoop();
    
    return 0;
}