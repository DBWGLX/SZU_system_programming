#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sstream>
#include "ThreadPool.hpp" // class Task
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务器线程任务

// 数据库配置宏
#define DB_HOST "tcp://127.0.0.1:3306"
#define DB_USER "root"
#define DB_PASSWORD "123456"
#define DB_NAME "chatServer"
#define DB_POOL_SIZE 5
//服务器端口
#define SERVER_PORT 8080

// Epoll 服务器类声明
class EpollServer {
public:
    EpollServer();
    ~EpollServer();
    void work(std::atomic<bool>& interrupted);

private:
    void initSocket();
    void handleEpollEvents(struct epoll_event* events, int readyFdCount);
    int serverFd;
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;
    MySQLConnectionPool mysqlPool;
    DBOperation dbop;
    LRUTokenManager LRUm;
};
