#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "ThreadPool.hpp" // class Task
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务器线程任务

// Epoll 服务器类声明
class EpollServer {
public:
    EpollServer();
    ~EpollServer();
    void work(std::atomic<bool>& interrupted);

private:
    void initSocket();
    int serverFd;
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;
    MySQLConnectionPool mysqlPool;
    DBOperation dbop;
    LRUTokenManager LRUm;
};
