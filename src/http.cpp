#include "http.h"

const char *ok_string = "<html><body></body></html>";
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";
int HttpConnect::user_count = 0;
int HttpConnect::epollfd = -1;

HttpConnect::HttpConnect() {

}

HttpConnect::~HttpConnect() {

}

void HttpConnect::DebugRead(char *input) {
    strcpy(read_buffer, input);
    read_index = strlen(input);
    ProcessRead();
    std::cout << "=== debug http read ===" << "\n";
    std::cout << "method: " << method << "\n";
    std::cout << "url: " << url << "\n";
    std::cout << "version: " << version << "\n";
    std::cout << "linger: " << linger << "\n";
    std::cout << "host: " << host << "\n";
    std::cout << "content_length: " << content_length << "\n";
    if (data == nullptr) data = "no-data";
    std::cout << "data: " << data << "\n";
    std::cout << "======" << "\n"; 
}

void HttpConnect::Init(int epollfd_, int sockfd_, const sockaddr_in &address_, char *doc_root_, int TRIGMode_) {
    epollfd = epollfd_;
    sockfd = sockfd_;
    address = address_;
    doc_root = doc_root_;
    TRIGMode = TRIGMode_;

    user_count++;

    Init();
}

void HttpConnect::Init() {
    bytes_have_send = 0;
    bytes_to_send = 0;
    write_index = 0;
    read_index = 0;
    check_index = 0;
    content_length = 0;
    start_line = 0;
    linger = false;
    check_state = CHECK_STATE_REQUESTLINE;
    method = GET;
    url = "\0";
    version = "\0";
    host = "\0";
    data = "\0";
    memset(read_buffer, '\0', READ_BUFFER_SIZE);
    memset(write_buffer, '\0', WRITE_BUFFER_SIZE);
}

void HttpConnect::Unmap() {
    if (file_address != nullptr) {
        munmap(file_address, file_stat.st_size);
        file_address = nullptr;
    }
}

bool HttpConnect::ReadOnce() {
    if (read_index >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;

    if (0 == TRIGMode) {
        bytes_read = recv(sockfd, read_buffer + read_index, READ_BUFFER_SIZE - read_index, 0);
        read_index += bytes_read;

        if (bytes_read <= 0) {
            return false;
        }

        return true;
    }
    else {
        while (true) {
            bytes_read = recv(sockfd, read_buffer + read_index, READ_BUFFER_SIZE - read_index, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (bytes_read == 0) {
                return false;
            }
            read_index += bytes_read;
        }
        return true;
    }

    return false;
}

void HttpConnect::CloseConnect(bool real_close) {
    if (real_close && (sockfd != -1)) {
        Removefd(epollfd, sockfd);
        sockfd = -1;
        user_count--;
    }
}

HttpConnect::HTTP_CODE HttpConnect::DoRequest() {
    strcpy(real_file, doc_root);
    const char *p = strrchr(url, '/');

    if (*(p+1) == '0') {
        strcat(real_file, "/judge.html");
    } else if (*(p+1) == '1') {
        strcat(real_file, "/t1.html");
    } else if (*(p+1) == '2') {
        strcat(real_file, "/t2.html");
    } else {
        strcat(real_file, url);
    }

    if (stat(real_file, &file_stat) < 0)
        return NO_RESOURCE;

    if (!(file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(file_stat.st_mode))
        return BAD_REQUEST;

    Log::GetInstance()->WriteLogDefault(0, "real file %s\n", real_file);

    int fd = open(real_file, O_RDONLY);
    file_address = (char *)mmap(0, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    return FILE_REQUEST;
}

HttpConnect::HTTP_CODE HttpConnect::ProcessRead() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    while ((check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = ParseLine()) == LINE_OK)) {
        text = GetLine();
        start_line = check_index;
        switch (check_state) {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = ParseRequestLine(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = ParseHeaders(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                else if (ret == GET_REQUEST) {
                    return DoRequest();
                }
                break;
            }
            case CHECK_STATE_CONTENT:
            {
                ret = ParseContent(text);
                if (ret == GET_REQUEST)
                    return DoRequest();
                line_status = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

HttpConnect::HTTP_CODE HttpConnect::ParseRequestLine(char *text) {
    url = strpbrk(text, " \t");
    if (!url) {
        return BAD_REQUEST;
    }
    *url++ = '\0';
    if (strncasecmp(text, "GET", 3) == 0) {
        method = GET;
    }
    else if (strncasecmp(text, "POST", 4) == 0) {
        method = POST;
    }
    else {
        return BAD_REQUEST;
    }
    url += strspn(url, " \t");
    version = strpbrk(url, " \t");
    if (!version) {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if (strncasecmp(version, "HTTP/1.1", 8) != 0) {
        return BAD_REQUEST;
    }
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }

    if (strncasecmp(url, "https://", 8) == 0) {
        url += 8;
        url = strchr(url, '/');
    }

    if (!url || url[0] != '/') {
        return BAD_REQUEST;
    }
    
    if (strlen(url) == 1) {
        strcat(url, "judge.html");
    }
    check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;   
}

HttpConnect::HTTP_CODE HttpConnect::ParseHeaders(char *text) {
    if (text[0] == '\0') {
        if (content_length != 0) {
            check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, "\t");
        if (strncasecmp(text, "keep-alive", 10) == 0) {
            linger = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host = text;
    }
    else {
        
    }
    return NO_REQUEST;
}

HttpConnect::HTTP_CODE HttpConnect::ParseContent(char *text) {
    if (read_index >= (content_length + check_index)) {
        text[content_length] = '\0';
        data = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpConnect::LINE_STATUS HttpConnect::ParseLine() {
    char temp;
    for (; check_index < read_index; ++check_index) {
        temp = read_buffer[check_index];
        if (temp == '\r') {
            if ((check_index + 1) == read_index) {
                return LINE_OPEN;
            }
            else if (read_buffer[check_index + 1] == '\n') {
                read_buffer[check_index++] = '\0';
                read_buffer[check_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n') {
            if (check_index > 1 && read_buffer[check_index - 1] == '\r') {
                read_buffer[check_index - 1] = '\0';
                read_buffer[check_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

bool HttpConnect::Write() {
    int temp = 0;
    int newadd = 0;
    Log::GetInstance()->WriteLogDefault(0, "[http_connect] bytes_to_send:%d\n", bytes_to_send);
    if (bytes_to_send == 0) {
        Modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
        Init();
        return true;
    }

    while (1) {
        temp = writev(sockfd, iv, iv_count);
        if (temp >= 0) {
            bytes_have_send += temp;
            newadd = bytes_have_send - write_index;
        } else {
            if (errno == EAGAIN) {
                if (bytes_have_send >= iv[0].iov_len) {
                    iv[0].iov_len = 0;
                    iv[1].iov_base = file_address + newadd;
                    iv[1].iov_len = bytes_to_send;
                }
                else {
                    iv[0].iov_base = write_buffer + bytes_to_send;
                    iv[0].iov_len = iv[0].iov_len - bytes_have_send;
                }
                Modfd(epollfd, sockfd, EPOLLOUT, TRIGMode);
                return true;
            }
            Unmap();
            return false;
        }
        bytes_to_send -= temp;
        if (bytes_to_send <= 0) {
            Unmap();
            Modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
            if (linger) {
                Init();
                return true;
            } else {
                return false;
            }
        }
    }
}

bool HttpConnect::ProcessWrite(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            AddStatusLine(500, error_500_title);
            AddHeaders(strlen(error_500_form));
            if (!AddContent(error_500_form)) {
                return false;
            }
            break;
        } 
        case BAD_REQUEST: {
            AddStatusLine(404, error_404_title);
            AddHeaders(strlen(error_404_form));
            if (!AddContent(error_404_form)) {
                return false;
            }
            break;
        } 
        case FORBIDDEN_REQUEST: {
            AddStatusLine(403, error_403_title);
            AddHeaders(strlen(error_403_form));
            if (!AddContent(error_403_form)) {
                return false;
            }
            break;    
        }
        case FILE_REQUEST: {
            Log::GetInstance()->WriteLogDefault(0, "[http_connect] st_size:%d\n", file_stat.st_size);
            AddStatusLine(200, ok_200_title);
            if (file_stat.st_size != 0) {
                AddHeaders(file_stat.st_size);
                iv[0].iov_base = write_buffer;
                iv[0].iov_len = write_index;
                iv[1].iov_base = file_address;
                iv[1].iov_len = file_stat.st_size;
                iv_count = 2;
                Log::GetInstance()->WriteLogDefault(0, "[http_connect] write_index:%d file_stat.st_size:%d\n", write_index, file_stat.st_size);
                bytes_to_send = write_index + file_stat.st_size;
                return true;
            } else {
                AddHeaders(strlen(ok_string));
                if (!AddContent(ok_string))
                    return false;
            }
        }
        default:
            return false;
    }
    iv[0].iov_base = write_buffer;
    iv[0].iov_len = write_index;
    iv_count = 1;
    bytes_to_send = write_index;
    return true;
}

bool HttpConnect::AddResponse(const char *format, ...) {
    Log::GetInstance()->WriteLogDefault(0, "[http_connect][AddResponse] write_index:%d\n", write_index);
    if (write_index >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(write_buffer + write_index, WRITE_BUFFER_SIZE - 1 - write_index, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - write_index)) {
        va_end(arg_list);
        return false;
    }
    write_index += len;
    va_end(arg_list);
    return true;
}

bool HttpConnect::AddStatusLine(int status, const char *title) {
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConnect::AddHeaders(int content_len) {
    return AddContentLength(content_len) && AddLinger() && AddBlankLine();
}
bool HttpConnect::AddContentLength(int content_len) {
    return AddResponse("Content-Length:%d\r\n", content_len);
}

bool HttpConnect::AddContentType() {
    return AddResponse("Content-Type:%s\r\n", "text/html");
}

bool HttpConnect::AddLinger() {
    return AddResponse("Connection:%s\r\n", (linger == true) ? "keep-alive" : "close");
}

bool HttpConnect::AddBlankLine() {
    return AddResponse("%s", "\r\n");
}

bool HttpConnect::AddContent(const char *content) {
    return AddResponse("%s", content);
}

void HttpConnect::Process() {
    Log::GetInstance()->WriteLogDefault(0, "[http_connect] Http process...\n");
    Log::GetInstance()->WriteLogDefault(0, "read_buffer: %s\n", read_buffer);
    HTTP_CODE read_ret = ProcessRead();
    Log::GetInstance()->WriteLogDefault(0, "==== read end ===\n");
    Log::GetInstance()->WriteLogDefault(0, "ret: %d\n", read_ret);
    Log::GetInstance()->WriteLogDefault(0, "url: %s\n", url);
    Log::GetInstance()->WriteLogDefault(0, "method: %d\n", method);
    Log::GetInstance()->WriteLogDefault(0, "version: %s\n", version);
    Log::GetInstance()->WriteLogDefault(0, "linger: %d\n", linger);
    Log::GetInstance()->WriteLogDefault(0, "host: %s\n", host);
    Log::GetInstance()->WriteLogDefault(0, "content_length: %d\n", content_length);
    
    if (read_ret == NO_REQUEST) {
        Modfd(epollfd, sockfd, EPOLLIN, TRIGMode);
        return;
    }
    bool write_ret = ProcessWrite(read_ret);
    Log::GetInstance()->WriteLogDefault(0, "==== write end ===\n");
    Log::GetInstance()->WriteLogDefault(0, "ret: %d\n", write_ret);

    if (! write_ret) {
        CloseConnect(true);
    }
    Modfd(epollfd, sockfd, EPOLLOUT, TRIGMode);
}