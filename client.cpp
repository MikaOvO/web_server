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
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h> 
#include <sys/uio.h>
#include <mutex>
#include <cstring>
#include <thread>
#include <fstream>
#include <ctime>

const char *ip = "127.0.0.1";
const int port = 8088;

void Run() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

    std::this_thread::sleep_for(std::chrono::seconds(3));

    char read_buffer[1024];
    char write_buffer[1024] = "GET\t/1.html\tHTTP/1.1\r\nAccept:image/gif.image/jpeg,*/*\r\nAccept-Language:zh-cn\r\nConnection:Keep-Alive\r\nHost:localhost\r\nUser-Agent:Mozila/4.0(compatible;MSIE5.01;Window NT5.0)\r\nAccept-Encoding:gzip,deflate\r\n\r\nusername=user&password=1234\0";

    int num = send(sockfd, write_buffer, strlen(write_buffer),0);
    std::cout << "send: " << num << "\n";
    num = recv(sockfd, read_buffer, sizeof(read_buffer),0);
    std::cout << "recv: " << num << "\n";
    std::cout << read_buffer << "\n";
}

int main() {
    Run();
    std::this_thread::sleep_for(std::chrono::seconds(1000000));
    return 0;
}