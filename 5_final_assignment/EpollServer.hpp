#pragma once
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <jansson.h>
#include <atomic>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "ThreadPool.h"
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"
#include "User.hpp"
#include "LRUTokenManager.hpp"

{//密码加密处理
    // 生成随机盐
    std::string generateSalt(size_t length = 16) {
        unsigned char salt[length];
        if (RAND_bytes(salt, length) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }
        return std::string(reinterpret_cast<char*>(salt), length);
    }
    // 使用 PBKDF2 生成哈希
    std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32) {
        unsigned char hash[key_len];

        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                            reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                            iterations, EVP_sha256(), key_len, hash) != 1) {
            throw std::runtime_error("Failed to hash password");
        }

        return std::string(reinterpret_cast<char*>(hash), key_len);
    }
    // 将二进制数据转为十六进制字符串，方便存储
    std::string toHex(const std::string& input) {
        std::ostringstream oss;
        for (unsigned char c : input) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return oss.str();
    }
}

//线程方法
class ClientTask : public Task {
public:
    ClientTask(int clientFd, int epollFd, DBOperation*dbopPtr)
        :_clientFd(clientFd),_epollFd(epollFd),_dbopPtr(dbopPtr)
    {}
    void execute() override {
        char buffer[4096];
        ssize_t bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);
        if(bytesRead <= 0){
            perror("recv Error");
            freeFd();
            return;
        }
        buffer[bytesRead] = '\0';

        json_t *root;
        json_error_t error;

        root = json_loads(buffer, 0, &error);
        if (!root) {
            std::cerr << "JSON parse error: " << error.text << std::endl;
            freeFd();
            return;
        }

        // 读取 type 字段
        json_t *type_json = json_object_get(root, "type");
        if (!json_is_integer(type_json)) {
            std::cerr << "Invalid 'type' field in JSON" << std::endl;
            json_decref(root);
            freeFd();
            return;
        }
        int type = json_integer_value(type_json);
        handleMessageType(type, root);//
        json_decref(root);
    }
private:
    void handleMessageType(int type, json_t *root) {
        switch (type) {
            case 1:
                handleType1(root);
                break;
            case 2:
                handleType2(root);
                break;
            default:
                std::cerr << "Unknown message type: " << type << std::endl;
                break;
        }
    }
    //发送处理结果
    void sendResult(int result) {
        std::string response = "{\"result\":" + std::to_string(result) + "}";
        send(clientFd, response.c_str(), response.size(), 0);
    }
    //3.发送在线用户名称
    void sendOnlineUsers(const std::vector<std::string>& users) {
        json_t *response = json_array();
        for (const auto& user : users) {
            json_array_append_new(response, json_string(user.c_str()));
        }
        char *responseStr = json_dumps(response, 0);
        if (responseStr) {
            send(clientFd, responseStr, strlen(responseStr), 0);
            free(responseStr);
        }
        json_decref(response);
    }
    void handleType1(json_t *root) {
        // 解析 account 字段
        json_t *account_json = json_object_get(root, "account");
        if (!json_is_string(account_json)) {
            std::cerr << "Invalid 'account' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *account = json_string_value(account_json);
        // 解析 password 字段
        json_t *password_json = json_object_get(root, "password");
        if (!json_is_string(password_json)) {
            std::cerr << "Invalid 'password' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *password = json_string_value(password_json);
        // 解析 username 字段
        json_t *username_json = json_object_get(root, "username");
        if (json_is_string(username_json)) {
            std::cerr << "Invalid 'username' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *username = json_string_value(username_json);
        // 解析 phone_number 字段
        json_t *phone_number_json = json_object_get(root, "phone_number");
        if (json_is_string(phone_number_json)) {
            std::cerr << "Invalid 'phone_number' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *phone_number = json_string_value(phone_number_json);
        // 解析 email 字段
        json_t *email_json = json_object_get(root, "email");
        if (json_is_string(email_json)) {
            std::cerr << "Invalid 'email' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *email = json_string_value(email_json);

        User user(account, password, username, phone_number, email);
        int res = dbop->addUser(user);
        sendResult(res);
    }
    void handleType2(json_t *root) {
        // 解析 account 字段
        json_t *account_json = json_object_get(root, "account");
        if (!json_is_string(account_json)) {
            std::cerr << "Invalid 'account' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *account = json_string_value(account_json);
        // 解析 password 字段
        json_t *password_json = json_object_get(root, "password");
        if (!json_is_string(password_json)) {
            std::cerr << "Invalid 'password' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *password = json_string_value(password_json);
        // 从数据库获取该账户的存储密码和盐
        bool res = dbop->verifyUser(account,password);
        if(res)
            sendResult(0);
        else
            sendResult(-1);
    }
    void handleType3(json_t *root){
        json_t *account_json = json_object_get(root, "account");
        if (!json_is_string(account_json)) {
            std::cerr << "Invalid 'account' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *account = json_string_value(account_json);
        // 解析 token 字段
        json_t *token_json = json_object_get(root, "token");
        if (!json_is_string(token_json)) {
            std::cerr << "Invalid 'token' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *token = json_string_value(token_json);
        // 验证 token
        if (!tokenManager.verifyToken(account, token)) {
            std::cerr << "Invalid or expired token" << std::endl;
            sendResult(-11);
            return;
        }
        // 获取在线用户列表
        std::vector<std::string> users = tokenManager.getAllUsers();
        sendOnlineUsers(users);
    }
    void handleType4(json_t *root){
        json_t *account_json = json_object_get(root, "account");
        if (!json_is_string(account_json)) {
            std::cerr << "Invalid 'account' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *account = json_string_value(account_json);
        // 解析 token 字段
        json_t *token_json = json_object_get(root, "token");
        if (!json_is_string(token_json)) {
            std::cerr << "Invalid 'token' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *token = json_string_value(token_json);
        // 验证 token
        if (!tokenManager.verifyToken(account, token)) {
            std::cerr << "Invalid or expired token" << std::endl;
            sendResult(-11);
            return;
        }
        // 解析 receiver_username 字段
        json_t *receiver_json = json_object_get(root, "receiver_username");
        if (!json_is_string(receiver_json)) {
            std::cerr << "Invalid 'receiver_username' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *receiver = json_string_value(receiver_json);
        // 解析 message 字段
        json_t *message_json = json_object_get(root, "message");
        if (!json_is_string(message_json)) {
            std::cerr << "Invalid 'message' field in JSON" << std::endl;
            sendResult(-10);
            return;
        }
        const char *message = json_string_value(message_json);
        // 获取接收者的文件描述符
        int receiver_fd = tokenManager.getUserFd(receiver);
        if (receiver_fd == -1) {
            std::cerr << "Receiver not online" << std::endl;
            sendResult(-12);
            return;
        }
        // 构造消息并发送
        std::string msg = "{\"sender\":\"" + std::string(account) + "\",\"message\":\"" + std::string(message) + "\"}";
        ssize_t bytes_sent = send(receiver_fd, msg.c_str(), msg.size(), 0);
        if (bytes_sent == -1) {
            std::cerr << "Failed to send message" << std::endl;
            sendResult(-13);  // 发送失败
            return;
        }

        // 等待接收确认
        std::string ack;
        if (receiveAcknowledgment(receiver_fd, ack)) {
            if (ack == "ACK") {
                sendResult(0);  // 成功
            } else {
                std::cerr << "Receiver did not acknowledge the message" << std::endl;
                sendResult(-14);  // 未收到确认
            }
        } else {
            std::cerr << "Failed to receive acknowledgment" << std::endl;
            sendResult(-15);  // 接收确认失败
        }
    }

    void freeFd(){
        epoll_event event;
        if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _clientFd, &event) == -1) {
            perror("epoll_ctl: EPOLL_CTL_DEL");
        }
        close(_clientFd);
    }
    int _clientFd;
    int _epollFd;
    DBOperation* _dbopPtr;
};

class EpollServer{
public:
    EpollServer():threadPool(std::make_unique<ThreadPool>(4)),
        mysqlPool("tcp://127.0.0.1:3306", "root", "123456", "chatServer", 5),
        dbop(&mysql)
    {
        initSocket();
    }
    ~EpollServer(){
        close(epollFd);
        close(serverFd);
    }
    void work(std::atomic<bool>& interrupted){
        while(!interrupted){
            struct epoll_event events[10];
            int readyFdCount = epoll_wait(epollFd, events, 10, -1);
            if(readyFdCount == -1){
                throw std::runtime_error("epoll_wait error");
                break;
            }

            for(int i=0;i<readyFdCount;i++){
                if(events[i].data.fd == serverFd){
                    //服务器套接字收到连接请求
                    sockaddr_in clientAddr{};
                    int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(clientAddr));
                    if (clientFd == -1) {
                        perror("accept");
                        continue;
                    }

                    // 监听该客户端套接字
                    epoll_event clientEvent{};
                    clientEvent.data.fd = clientFd;
                    clientEvent.events = EPOLLIN;
                    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1){
                        perror("epoll_ctl");
                        close(clientFd);
                    }
                }
                else{ //收到客户端数据
                    threadPool->enqueue(new ClientTask(events[i].data.fd, epollFd, &dbop));
                }
            }
        }
    }
private:
    void initSocket(){
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if(serverFd == -1){
            perror("socket");
            throw std::runtime_error("Failed to create socket");
        }
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(8080);
        // 绑定套接字
        if(bind(serverFd,reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
            close(serverFd);
            throw std::runtime_error("Failed to bind socket");
        }
        // 监听套接字
        if(listen(serverFd, SOMAXCONN) == -1){
            close(serverFd);
            throw std::runtime_error("Failed to listen on socket");
        }
        // 创建 epoll 实例
        epollFd = epoll_create1(0);
        if(epollFd == -1){
            close(serverFd);
            throw std::runtime_error("Failed to create epoll instance");
        }
        // 将服务器套接字添加到 epoll 实例中, 来监听它~
        epoll_event event{};
        event.data.fd = serverFd;
        event.events = EPOLLIN;
        if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1){
            close(epollFd);
            close(serverFd);
            throw std::runtime_error("Failed to add server socket to epoll");
        }
    }

    int serverFd;//套接字
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;
    MySQLConnectionPool mysqlPool;
    DBOperation dbop;
    LRUTokenManager lruTM;
};