#include "log.hpp"

const char* Log::level_info[4] = {
    "[debug]  ", 
    "[info]   ",
    "[warning]",
    "[error]  "
};
const char* Log::default_file_name = "log";
const char* Log::dir = "/home/mika/workspace/cpp_workspace/web_server/log";

Log::Log() {
    current_file = nullptr;
}

Log::~Log() {
    if (current_file != nullptr) {
        file_stream.close();
    }
}