#include <iostream>
#include <thread>
#include <assert.h>

#include "safe_queue.h"
#include "thread_pool.hpp"
#include "log.hpp"
#include "http.h"
#include "timer.h"

void Push1000NumberToQueue(SafeQueue<int> &q) {
    for (int i = 0; i < 1000; ++i) {
        q.push(i);
    }
}

void TestSafeQueue() {
    SafeQueue<int> q;
    std::thread t1(Push1000NumberToQueue, std::ref(q));
    std::thread t2(Push1000NumberToQueue, std::ref(q));
    std::thread t3(Push1000NumberToQueue, std::ref(q));
    t1.join();
    t2.join();
    t3.join();
    assert(q.size() == 3000);
}

int Add(int x, int y) {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return x + y;
}

int Multiply(int x, int y, int &z) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    z = x * y;
    return z;
}

void TestThreadPool() {
    ThreadPool tp(8);
    tp.Run();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int ref_mul1, ref_mul2;
    auto future_add1 = tp.Submit(Add, 3, 2);
    auto future_add2 = tp.Submit(Add, 2, 2);
    auto future_mul1 = tp.Submit(Multiply, 10, 2, std::ref(ref_mul1));
    auto future_mul2 = tp.Submit(Multiply, 10, 200, std::ref(ref_mul2));
    auto future_add3 = tp.Submit(Add, 100, 2);
    int add1 = future_add1.get();
    int add2 = future_add2.get();
    int add3 = future_add3.get();
    int mul1 = future_mul1.get();
    int mul2 = future_mul2.get();
    assert(add1 == 3 + 2);
    assert(add2 == 2 + 2);
    assert(add3 == 100 + 2);
    assert(mul1 == 10 * 2);
    assert(mul2 == 10 * 200);
    assert(mul1 == ref_mul1);
    assert(mul2 == ref_mul2);
}

void TestLog() {
    Log::GetInstance()->WriteLogDefault(0, "%d-%.1lf", 3, 10.5);
    Log::GetInstance()->WriteLogDefault(1, "%s-%s", "aaa", "bbb");
}

void TestHttp() {
    // GET /sample.jsp HTTP/1.1
    // Accept:image/gif.image/jpeg,*/*
    // Accept-Language:zh-cn
    // Connection:Keep-Alive
    // Host:localhost
    // User-Agent:Mozila/4.0(compatible;MSIE5.01;Window NT5.0)
    // Accept-Encoding:gzip,deflate
    
    // username=user&password=1234
    char *input = "GET\t/sample.jsp\tHTTP/1.1\r\nAccept:image/gif.image/jpeg,*/*\r\nAccept-Language:zh-cn\r\nConnection:Keep-Alive\r\nHost:localhost\r\nUser-Agent:Mozila/4.0(compatible;MSIE5.01;Window NT5.0)\r\nAccept-Encoding:gzip,deflate\r\n\r\nusername=user&password=1234\0";
    HttpConnect hc;
    hc.Init();
    hc.DebugRead(input);
}

int main() {
    // TestHttp();
    TestLog();
    TestSafeQueue();
    // TestThreadPool();
    return 0;
}