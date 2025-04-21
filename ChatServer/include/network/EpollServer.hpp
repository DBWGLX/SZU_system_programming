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
    void work();

private:
    void initSocket();
    int serverFd;
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;

    LRUTokenManager _LRUm;
    std::atomic<bool>& _interrupted;

    //update
    io_uring ring;
    void submitAccept(AcceptContext* ctx);
    void submitRead(int clientFd, ReadContext* ctx);
    void submitSend(SendContext* ctx);
};
