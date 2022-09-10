#pragma once

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
#include <iostream>

#include "epoll_utils.h"
#include "log.hpp"
#include "timer.h"

class HttpConnect
{
public:
    const static int READ_BUFFER_SIZE = 2048;
    const static int WRITE_BUFFER_SIZE = 1024;
    const static int FILE_SIZE = 256;

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
public:
    static int epollfd;
    static int user_count;

public:
    HttpConnect();
    ~HttpConnect();

    char * GetLine() { return read_buffer + start_line; };

    void Init(int epollfd_, int sockfd_, const sockaddr_in &address_, char *doc_root_, int TRIGMode_);
    void Init();

    void Process();
    bool ReadOnce();

    bool Write();

    void Unmap();

    void CloseConnect(bool real_close);

    HTTP_CODE ProcessRead();

    HTTP_CODE ParseRequestLine(char *text);
    HTTP_CODE ParseHeaders(char *text);
    HTTP_CODE ParseContent(char *text);

    LINE_STATUS ParseLine();
    HTTP_CODE DoRequest();

    bool ProcessWrite(HTTP_CODE ret);
    bool AddResponse(const char *format, ...);
    bool AddContent(const char *content);
    bool AddStatusLine(int status, const char *title);
    bool AddHeaders(int content_length);
    bool AddContentType();
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();

    void DebugRead(char *input); 

private:
    int sockfd;
    sockaddr_in address;
    int TRIGMode;
    char *doc_root;
    char *file_address;
    struct stat file_stat;

    size_t check_index;
    size_t read_index;
    char read_buffer[READ_BUFFER_SIZE];
    size_t write_index;
    char write_buffer[WRITE_BUFFER_SIZE];
    char real_file[FILE_SIZE];

    int start_line;

    size_t content_length;

    bool linger;

    char *url;
    char *version;
    CHECK_STATE check_state;
    METHOD  method;
    char *host;
    char *data;

    int bytes_to_send;
    int bytes_have_send;

    struct iovec iv[2];
    int iv_count;
};
