#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
//#include <jansson.h>
#include <atomic>
#include <iomanip>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "ThreadPool.hpp"
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务
#include "PasswordUtils.hpp"//密码加密方法

#define PROTOBUF_KEY 0x12345678  // K固定为0x12345678

// 线程任务类声明
class ClientTask : public Task {
public:
    ClientTask(int clientFd, int epollFd, DBOperation* dbopPtr, LRUTokenManager* LRUm);
    void execute() override;//默认操作；先解析
private:
    void PROTOBUF_handleMessageType(int type, json_t *root);
    ssize_t sendAll(int sockfd, const char* message);
    ssize_t sendResult(int sockfd, int type, const char* message);
    int sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users);
    void PROTOBUF_handleType1(json_t *root);//注册
    void PROTOBUF_handleType2(json_t *root);//登录
    void PROTOBUF_handleType3(json_t *root);//获取在线用户列表
    void PROTOBUF_handleType4(json_t *root);//聊天
    void PROTOBUF_handleType5(json_t *root);//登出
    void JSON_handleType1(json_t *root);//注册
    void JSON_handleType2(json_t *root);//登录
    void JSON_handleType3(json_t *root);//获取在线用户列表
    void JSON_handleType4(json_t *root);//聊天
    void JSON_handleType5(json_t *root);//登出
    void freeFd();
    int _clientFd;
    int _epollFd; //断开连接时删除用
    DBOperation* _dbopPtr;
    LRUTokenManager* _LRUm;//manager
};
