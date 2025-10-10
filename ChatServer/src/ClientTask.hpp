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
#include <liburing.h>
#include "ThreadPool.hpp"
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"
#include "ClientTask.hpp"//服务
#include "PasswordUtils.hpp"//密码加密方法
#include "ContextType.hpp"
#include "chat.pb.h"
#define PROTOBUF_KEY 0x12345678  // K固定为0x12345678

using namespace dbwg;

// 线程任务类声明
class ClientTask : public Task {
public:
    ClientTask(int clientFd,std::string msg, int epollFd, LRUTokenManager* LRUm, io_uring* ring);
    void execute(DBOperation& dbop) override;//默认操作；先解析序列化协议
private:

    ///protocbuf
    //1.处理逻辑
    void PROTOBUF_handle();//protobuf协议处理逻辑
    //根据消息类型自动选择处理逻辑
    void PROTOBUF_handleMessageType(int type, const std::string& msg);
    //不同格式消息处理逻辑
    void PROTOBUF_handleType1(const std::string& received_data);//注册
    void PROTOBUF_handleType2(const std::string& received_data);//登录
    void PROTOBUF_handleType3(const std::string& received_data);//获取在线用户列表
    void PROTOBUF_handleType4(const std::string& received_data);//聊天
    void PROTOBUF_handleType5(const std::string& received_data);//登出

    //2.转发逻辑
    //2a. io_uring 发送策略 【批量操作，应额外设计】
    size_t submitSend(SendContext* ctx) ;
    //2b. 直接套接字发送
    ssize_t sendAll(int sockfd, const char* data, size_t len);
    size_t PROTOBUF_sendAll(int sockfd, const std::string& serialized_data);//流前添加 key len
    //2b.1 protocbuf 流式序列化
    ssize_t PROTOBUF_sendResult(int sockfd, int type, const char* message);
    ssize_t PROTOBUF_sendLoginResult(int sockfd, std::string& username, std::string& token);
    ssize_t PROTOBUF_sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users);
    ssize_t PROTOBUF_sendChatMessage(int sockfd, const std::string& account, const std::string& message);


    /////
    void freeFd();
    int _clientFd;//消息所属身份
    std::string _msg;
    int _epollFd; //断开连接时用

    LRUTokenManager* _LRUm;//manager    //【高耦合】考虑 "provider"文件作为容器列表 供所有人访问
    DBOperation* _dbopPtr;
    std::chrono::seconds timeoutSeconds;//recv超时时间
    io_uring* _ring;
};
