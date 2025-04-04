#include <iostream>
#include <cstdio>
#include <thread>
#include <atomic>
#include <cstring>
#include <sstream>
#include <mutex>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>
#include "chat.pb.h"


#include <vector>
#define SERVER_PORT 8080

// 字体颜色
#define COLOR_BLACK  "\033[30m"
#define COLOR_RED    "\033[31m"
#define COLOR_MAGENTA "\033[35m" //洋红
#define COLOR_YELLOW "\033[33m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_CYAN   "\033[36m" //蓝绿色
// 背景颜色宏定义
#define BG_COLOR_BLACK  "\033[40m"
#define BG_COLOR_RED    "\033[41m"
#define BG_COLOR_GREEN  "\033[42m"
#define BG_COLOR_YELLOW "\033[43m"
#define BG_COLOR_BLUE   "\033[44m"
#define BG_COLOR_MAGENTA "\033[45m"
#define BG_COLOR_CYAN   "\033[46m"
#define BG_COLOR_WHITE  "\033[47m"

#define COLOR_RESET  "\033[0m"

#define CommandLinePrompt "\033[32m[ChatClient]$\033[0m "

#define PROTOBUF_KEY 0x12345678  // K固定为0x12345678

std::atomic<bool> running(true);

struct UserInfo {
    std::mutex mutex;
    std::string account;
    std::string token;
    std::string username;
    bool is_logged_in = false;

    void setToken(const std::string &tkn) {
        std::lock_guard<std::mutex> lock(mutex);
        token = tkn;
        is_logged_in = true;
    }

    void logout() {
        std::lock_guard<std::mutex> lock(mutex);
        account.clear();
        token.clear();
        is_logged_in = false;
    }

    std::pair<std::string, std::string> getCredentials() {
        std::lock_guard<std::mutex> lock(mutex);
        return {account, token};
    }
};

std::string now(){
    std::time_t now = std::time(nullptr);
    std::tm* local_tm = std::localtime(&now);
    std::stringstream str;
    str << (local_tm->tm_year+1900) << "-"
        << std::setw(2) << std::setfill('0') << (local_tm->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << local_tm->tm_mday << " "
        << std::setw(2) << std::setfill('0') << local_tm->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << local_tm->tm_min << ":"
        << std::setw(2) << std::setfill('0') << local_tm->tm_sec;
    return str.str();
}

ssize_t sendAll(int sockfd, const char* data, size_t len) {
    size_t totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = send(sockfd, data + totalSent, len - totalSent, 0);
        
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);  // 发送缓冲区满时，等待 1ms 再尝试
                perror("send try again");
                continue;
            } else {
                perror("send failed");
                return -1;  // 发送失败
            }
        } else if (sent == 0) {
            std::cerr << "连接关闭！" << std::endl;
            return -1;
        }

        totalSent += sent;
    }
    return totalSent;
}
ssize_t PROTOBUF_sendAll(int sockfd, std::string serialized_data){// KLV打包
    uint32_t key = htonl(PROTOBUF_KEY);
    uint32_t length = htonl(serialized_data.size());
    std::string packet;
    packet.append(reinterpret_cast<const char*>(&key), sizeof(key));  // K
    packet.append(reinterpret_cast<const char*>(&length), sizeof(length));  // L
    packet.append(serialized_data);  // V（Protobuf 数据）
    return sendAll(sockfd, packet.data(), packet.size());
}

//接收端
void receiveMessage(std::vector<int>& sockfds, UserInfo* user_info) {
    fd_set readfds;//select
    int max_fd = 0;
    // 找出最大的文件描述符
    for (int fd : sockfds) {
        if (fd > max_fd) {
            max_fd = fd;
        }
    }

    while (running) {
        // 清空文件描述符集合
        FD_ZERO(&readfds);

        // 将所有文件描述符添加到集合中
        for (int fd : sockfds) {
            FD_SET(fd, &readfds);
        }

        // 调用 select 函数监听文件描述符
        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            continue;
        }

        // 检查哪些文件描述符有数据可读
        for (int fd : sockfds) {
            if (FD_ISSET(fd, &readfds)) {
            int sockfd = fd;
            uint32_t key;
            size_t bytes_received = recv(sockfd, &key, sizeof(key), 0);
            if (bytes_received > 0){ 
                key = ntohl(key); //PROTOBUF_KEY

                uint32_t length;
                if (recv(sockfd, &length, sizeof(length), 0) <= 0) continue;
                length = ntohl(length);

                //读取
                std::string received_data(length, '\0');
                if (recv(sockfd, &received_data[0], length, 0) <= 0) continue;

                chat::Response msg; // 只用一下type字段
                if (!msg.ParseFromString(received_data)) {
                    perror("recv: Failed to parse protobuf message!");
                    continue;
                }

                int type = msg.type();
                std::cout << std::endl;
                switch (type) {
                    // 注册响应处理
                    case 1001: {
                        std::cout << BG_COLOR_GREEN << "注册成功: " << msg.message()
                            << COLOR_RESET << std::endl;
                        break;
                    }
                    case 1002: {
                        std::cout << BG_COLOR_RED << "注册失败: " << msg.message()
                            << COLOR_RESET << std::endl;
                        break;
                    }
                    case 2001: { // 登录成功
                        chat::LoginResponse loginmsg;
                        if (!loginmsg.ParseFromString(received_data)) {
                            perror("login: Failed to parse protobuf message!");
                            continue;
                        }
                        user_info->setToken(loginmsg.token());
                        user_info->username = loginmsg.username();

                        std::cout << BG_COLOR_GREEN  << "登录成功" << COLOR_RESET  << std::endl;
                        std::cout << COLOR_YELLOW << "欢迎！" << user_info->username << COLOR_RESET << std::endl;
                        break;
                    }
                    case 2002: // 登录失败
                        user_info->logout();
                        std::cout << BG_COLOR_RED  << "登录失败。" << COLOR_RESET  << std::endl;
                        break;
                    case 3001: { // 在线用户列表
                        chat::GetOnlineUsersResponse response;
                        if (response.ParseFromString(received_data)) {
                            std::cout << "Received Online Users List:\n";
                            size_t index = 0;
    
                            for (const auto& user : response.users()) {
                                std::cout << index++ << ". " << "Name: " << COLOR_YELLOW << user.name() << COLOR_RESET 
                                << ", Account: " << user.account() << std::endl;
                            }
                        }else{
                            perror("online: Failed to parse Protobuf message!");
                        }
                        break;
                    }
                    case 3002:{
                        std::cout << std::endl << msg.message() << std::endl;
                        break;
                    }
                    // 消息发送状态处理
                    case 4001: {
                        std::cout << "✓ 消息已送达: " << msg.message() << std::endl;
                        break;
                    }
                    case 4002: {
                        std::cout << "✗ 发送失败: " << msg.message() << std::endl;
                        break;
                    }
                    // 接收消息处理
                    case 4010: {
                        chat::ReceivedMessage rmsg;
                        if(rmsg.ParseFromString(received_data)){
                            if(rmsg.account() != user_info->account)
                                std::cout << COLOR_BLUE << "[来自 " << rmsg.account() << " 的消息] " << std::endl;
                            std::cout << COLOR_RESET << rmsg.message() << std::endl;
                        }
                        else {
                            std::cerr << "收到格式错误的消息" << std::endl;
                        }
                        break;
                    }
                    case 5001:
                        user_info->logout();
                        std::cout << "下线成功。" << std::endl;
                        break;
                    default:
                        std::cout << "[Server]: " << type << std::endl;
                        break;
                }
            } else if (bytes_received == 0) {
                std::cout << std::endl << "# 服务器关闭了连接。" << std::endl;
                running = false;
                break;
            } else {
                perror("recv失败");
                running = false;
                break;
            }
            }
            std::cout << CommandLinePrompt << std::flush;  // 保持输入提示可见
        }
    }
}

void thread_sender(int sockfd, int startn){
    for(int i=startn;i<startn + 2500;i++){
        if(i%500 == 0){
            std::cout << "now sending: " << i << std::endl;
        }

        std::string account = std::to_string(i);
        std::string password = std::to_string(i);
        std::string username = std::to_string(i);
        std::string phone = std::to_string(i);
        std::string email = std::to_string(i) + "@gmail.com";

        chat::RegisterRequest msg;
        msg.set_type(1000);
        msg.set_account(account);
        msg.set_password(password);
        msg.set_username(username);
        msg.set_phone_number(phone);
        msg.set_email(email);

        std::string serialized_data;
        if(!msg.SerializeToString(&serialized_data)){
            perror("register:Protobuf 序列化失败！");
            return;
        }

        if(PROTOBUF_sendAll(sockfd, serialized_data) <= 0){
            perror("register: send error");
        }
    }
}

int main() {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    std::vector<int> sockfds;
    for(int i=0;i<4;i++){
        sockfds.push_back(socket(AF_INET, SOCK_STREAM, 0));
        if(sockfds[i] == -1){
            std::cout << "num:" << i << " ";
            perror("socket创建失败");
            return -1;
        }
        if(connect(sockfds[i], reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1){
            std::cout << "num:" << i << " ";
            perror("连接失败");
            close(sockfds[i]);
            return 1;
        }
    }

    std::string time_start = now();

    UserInfo user_info;
    //std::thread sender(sendMessage, sockfd, &user_info);
    std::thread receiver(receiveMessage, std::ref(sockfds), &user_info);
    std::vector<std::thread> threads;
    for(int i=0;i<4;i++)
        threads.emplace_back(thread_sender, sockfds[i], i*2500);

    //sender.join();
    receiver.join();
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::cout << time_start << std::endl;
    //sleep(10000);

    //close(sockfd);
    return 0;
}