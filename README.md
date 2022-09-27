# web_server
Linux下C++轻量级Web服务器

* 实现线程池，可以提交任意非类成员函数执行（可以利用lambda简单封装一下来提交类成员函数: [](Object &o){ o.func(); }(std::ref(o));）
* 利用epoll(ET和LT可选)实现IO多路复用，由主线程进行请求分类和socket文件读写，工作线程进行解析和逻辑处理
* 利用状态机解析HTTP请求
* 利用小根堆+unordered_map，实现定时器，处理不活跃连接
* 实现同步日志系统

运行方法：
```
cd build
cmake .. && make
./server
```

压测：
ET非阻塞：
![image](https://user-images.githubusercontent.com/93330615/192477919-5403dcb0-49b7-4d95-b632-59281db94996.png)
