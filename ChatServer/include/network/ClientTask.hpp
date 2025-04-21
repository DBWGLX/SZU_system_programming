#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <stdio.h>
//#include <jansson.h>
#include <atomic>
#include <iomanip>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <chrono>
#include "ThreadPool.hpp"
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务
#include "PasswordUtils.hpp"//密码加密方法
#include "chat.pb.h"
#define PROTOBUF_KEY 0x12345678  // K固定为0x12345678

// 线程任务类声明
class ClientTask : public Task {
public:
    ClientTask(int clientFd,std::string msg, int epollFd, LRUTokenManager* LRUm);
    void execute(DBOperation& dbop) override;//默认操作；先解析序列化方式
private:
    void PROTOBUF_handle();//解析报文类型
    ssize_t sendAll(int sockfd, const char* data, size_t len);//发送策略
    ssize_t PROTOBUF_sendAll(int sockfd, std::string serialized_data);
    ssize_t PROTOBUF_sendResult(int sockfd, int type, const char* message);
    ssize_t PROTOBUF_sendLoginResult(int sockfd, std::string& username, std::string& token);
    ssize_t PROTOBUF_sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users);
    ssize_t PROTOBUF_sendChatMessage(int sockfd, const std::string& account, const std::string& message);
    void PROTOBUF_handleMessageType(int type, const std::string& msg);
    void PROTOBUF_handleType1(const std::string& received_data);//注册
    void PROTOBUF_handleType2(const std::string& received_data);//登录
    void PROTOBUF_handleType3(const std::string& received_data);//获取在线用户列表
    void PROTOBUF_handleType4(const std::string& received_data);//聊天
    void PROTOBUF_handleType5(const std::string& received_data);//登出
    void freeFd();
    int _clientFd;
    std::string _msg;
    int _epollFd; //断开连接时用
    LRUTokenManager* _LRUm;//manager
    DBOperation* _dbopPtr;
    std::chrono::seconds timeoutSeconds;//recv超时时间
};
