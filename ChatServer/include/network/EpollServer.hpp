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
#include "ThreadPool.hpp" // class Task
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务器线程任务

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
    void handleEpollEvents(struct epoll_event* events, int readyFdCount);
    int serverFd;
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;

    LRUTokenManager LRUm;
    std::atomic<bool>& _interrupted;
};
