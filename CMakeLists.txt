cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17) 

project(server)

add_subdirectory(./src web_server_binary_dir)

ADD_EXECUTABLE(server main.cpp)

set_target_properties(server PROPERTIES
COMPILE_FLAGS "-pthread"
LINK_FLAGS "-pthread")

target_link_libraries(server WebServer)
target_include_directories(server PRIVATE ./src)