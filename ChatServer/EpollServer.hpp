#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <jansson.h>
#include <atomic>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "ThreadPool.hpp" // class Task
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"

// 密码加密处理方法
std::string generateSalt(size_t length = 16);
std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32);
std::string toHex(const std::string& input);
std::string fromHex(const std::string& input);

// 线程任务类声明
class ClientTask : public Task {
public:
    ClientTask(int clientFd, int epollFd, DBOperation* dbopPtr, LRUTokenManager* LRUm);
    void execute() override;
private:
    void handleMessageType(int type, json_t *root);
    ssize_t sendAll(int sockfd, const char* message);
    ssize_t sendResult(int sockfd, int type, const char* message);
    int sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users);
    void handleType1(json_t *root);
    void handleType2(json_t *root);
    void handleType3(json_t *root);
    void handleType4(json_t *root);
    void handleType5(json_t *root);
    void freeFd();
    int _clientFd;
    int _epollFd;
    DBOperation* _dbopPtr;
    LRUTokenManager* _LRUm;
};

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
