cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17) 

project(test)

add_subdirectory(../src web_server_binary_dir)

ADD_EXECUTABLE(test main.cpp)

set_target_properties(test PROPERTIES
COMPILE_FLAGS "-pthread"
LINK_FLAGS "-pthread")

target_link_libraries(test WebServer)
target_include_directories(test PRIVATE ../src)