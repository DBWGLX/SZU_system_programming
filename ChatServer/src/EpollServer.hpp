/*


*/
#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sstream>
#include <set>
#include <atomic>
#include <liburing.h>
#include "ThreadPool.hpp" // class Task
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务器线程任务
#include "ContextType.hpp"
using namespace dbwg;

//服务器端口
#define SERVER_PORT 8080

// Epoll 服务器类声明
class EpollServer {
public:
    EpollServer(std::atomic<bool>& interrupted);
    ~EpollServer();
    void work();//运行逻辑

private:
    //服务标识
    void initSocket();
    int serverFd;
    int epollFd;

    //线程池逻辑
    std::unique_ptr<ThreadPool> threadPool;

    //缓存设计
    LRUTokenManager _LRUm;
    //服务停止
    std::atomic<bool>& _interrupted;

    //io_uring操作
    io_uring ring;
    void submitAccept(AcceptContext* ctx);
    void submitRead(int clientFd, ReadContext* ctx);
    void submitSend(SendContext* ctx);
};
