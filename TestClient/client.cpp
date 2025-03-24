#include <iostream>
#include <cstdio>
#include <thread>
#include <atomic>
#include <cstring>
#include <vector>
#include <sstream>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jansson.h>
#include <iomanip>

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

//操作端
void sendMessage(int sockfd, UserInfo *user_info) {
    printf("# 输入'/help'以获取帮助\n");

    while (running) {
        printf(CommandLinePrompt);
        std::string command;
        std::getline(std::cin, command);

        if (command.empty()) continue;

        if (command == "/quit" || command == "/q") {
            auto [current_account, current_token] = user_info->getCredentials();
            if (current_account.empty() || current_token.empty()) {
                std::cout << "未登录，无法发送下线请求。" << std::endl;
                continue;
            }

            json_t *root = json_object();
            json_object_set_new(root, "type", json_integer(5000));
            json_object_set_new(root, "account", json_string(current_account.c_str()));
            json_object_set_new(root, "token", json_string(current_token.c_str()));
            
            char *json_str = json_dumps(root, JSON_COMPACT);
            if(send(sockfd, json_str, strlen(json_str), 0) == -1){
                perror("send error");
            }
            
            free(json_str);
            json_decref(root);

            running = false;
            shutdown(sockfd, SHUT_RD);
            break;
        } else if (command == "/register" || command == "/r") {
            std::string account, password, username, phone, email;
            std::cout << "请输入账号: ";
            std::getline(std::cin, account);
            std::cout << "请输入密码: ";
            std::getline(std::cin, password);
            std::cout << "请输入用户名: ";
            std::getline(std::cin, username);
            std::cout << "请输入电话号码: ";
            std::getline(std::cin, phone);
            std::cout << "请输入邮箱: ";
            std::getline(std::cin, email);

            json_t *root = json_object();
            json_object_set_new(root, "type", json_integer(1000));
            json_object_set_new(root, "account", json_string(account.c_str()));
            json_object_set_new(root, "password", json_string(password.c_str()));
            json_object_set_new(root, "username", json_string(username.c_str()));
            json_object_set_new(root, "phone_number", json_string(phone.c_str()));
            json_object_set_new(root, "email", json_string(email.c_str()));
            
            char *json_str = json_dumps(root, JSON_COMPACT);
            if(send(sockfd, json_str, strlen(json_str), 0) == -1){
                perror("send error");
            }
            free(json_str);
            json_decref(root);
        } else if (command == "/login" || command == "/l") {
            std::string account, password;
            std::cout << "请输入账号: ";
            std::getline(std::cin, account);
            std::cout << "请输入密码: ";
            std::getline(std::cin, password);

            {
                std::lock_guard<std::mutex> lock(user_info->mutex);
                user_info->account = account;
            }

            json_t *root = json_object();
            json_object_set_new(root, "type", json_integer(2000));
            json_object_set_new(root, "account", json_string(account.c_str()));
            json_object_set_new(root, "password", json_string(password.c_str()));
            
            char *json_str = json_dumps(root, JSON_COMPACT);
            if(send(sockfd, json_str, strlen(json_str), 0) == -1){
                perror("send error");
            }
            free(json_str);
            json_decref(root);
        } else if (command == "/online" || command == "/o") {
            auto [current_account, current_token] = user_info->getCredentials();
            if (current_account.empty() || current_token.empty()) {
                std::cout << "请先登录！" << std::endl;
                continue;
            }

            json_t *root = json_object();
            json_object_set_new(root, "type", json_integer(3000));
            json_object_set_new(root, "account", json_string(current_account.c_str()));
            json_object_set_new(root, "token", json_string(current_token.c_str()));
            
            char *json_str = json_dumps(root, JSON_COMPACT);
            if(send(sockfd, json_str, strlen(json_str), 0) == -1){
                perror("send error");
            }
            free(json_str);
            json_decref(root);
        } else if (command == "/send" || command == "/s") {
            auto [current_account, current_token] = user_info->getCredentials();
            if (current_account.empty() || current_token.empty()) {
                std::cout << "请先登录！" << std::endl;
                continue;
            }

            std::string receiver, message;
            std::cout << "请输入接收者账号: ";
            std::getline(std::cin, receiver);
            std::cout << "请输入消息内容: ";
            std::getline(std::cin, message);
            message = "[" + now() + "] " + COLOR_YELLOW + user_info->username + COLOR_RESET + ": " + message;

            json_t *root = json_object();
            json_object_set_new(root, "type", json_integer(4000));
            json_object_set_new(root, "account", json_string(current_account.c_str()));
            json_object_set_new(root, "token", json_string(current_token.c_str()));
            json_object_set_new(root, "receiver_useraccount", json_string(receiver.c_str()));
            json_object_set_new(root, "message", json_string(message.c_str()));
            
            char *json_str = json_dumps(root, JSON_COMPACT);
            if(send(sockfd, json_str, strlen(json_str), 0) == -1){
                perror("send error");
            }
            free(json_str);
            json_decref(root);
        } else if (command == "/help" || command == "/h"){
            std::cout << "/register: " << "注册" 
            << std::endl << "/login: " << "登录"
            << std::endl << "/online: " << "查看当前在线用户"
            << std::endl << "/send: " << "发消息"
            << std::endl << "/quit: " << "退出"
            << std::endl;
        } else {
            std::cout << "未知命令。输入'/help'以获取帮助。" << std::endl;
        }
    }
}

//接收端
void receiveMessage(int sockfd, UserInfo *user_info) {
    char buffer[1024];
    while (running) {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            json_error_t error;
            json_t *root = json_loads(buffer, 0, &error);

            if (!root) {
                fprintf(stderr, "JSON解析错误: %s\n", error.text);
                continue;
            }

            json_t *type_obj = json_object_get(root, "type");
            if (!json_is_integer(type_obj)) {
                fprintf(stderr, "无效的type字段\n");
                json_decref(root);
                continue;
            }

            int type = json_integer_value(type_obj);
            switch (type) {
                // 注册响应处理
                case 1001: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << std::endl << BG_COLOR_GREEN << "注册成功: " << json_string_value(msg_obj) 
                            << COLOR_RESET << std::endl;
                    }
                    break;
                }
                case 1002: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << std::endl << BG_COLOR_RED << "注册失败: " << json_string_value(msg_obj) 
                            << COLOR_RESET << std::endl;
                    }
                    break;
                }
                case 2001: { // 登录成功
                    json_t *token_obj = json_object_get(root, "message");
                    if (json_is_string(token_obj)) {
                        const char *token = json_string_value(token_obj);
                        user_info->setToken(token);
                        std::cout << std::endl << BG_COLOR_GREEN  << "登录成功" << COLOR_RESET  << std::endl;
                    }
                    break;
                }
                case 2002: // 登录失败
                    user_info->logout();
                    std::cout << std::endl << BG_COLOR_RED  << "登录失败。" << COLOR_RESET  << std::endl;
                    break;
                case 2011:{
                    json_t *token_obj = json_object_get(root, "message");
                    if (json_is_string(token_obj)) {
                        const char *token = json_string_value(token_obj);
                        {
                            std::lock_guard<std::mutex> lock(user_info->mutex);
                            user_info->username = token;
                        }
                        std::cout << std::endl << COLOR_YELLOW << "欢迎！" << token << COLOR_RESET << std::endl;
                    }
                    break;
                }
                case 3001: { // 在线用户列表
                    json_t *users = json_object_get(root, "users");
                    if (json_is_array(users)) {
                        std::cout << std::endl << "在线用户列表：" << std::endl;
                        size_t index;
                        json_t *value;
                        json_array_foreach(users, index, value) {
                            const char *name = json_string_value(json_object_get(value, "name"));
                            const char *account = json_string_value(json_object_get(value, "account"));
                            if (name && account) {
                                std::cout << index << ". " << "用户名: " << COLOR_YELLOW << name << COLOR_RESET 
                                << ", 账号: " << account << std::endl;
                            }
                        }
                    }
                    break;
                }
                // 消息发送状态处理
                case 4001: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << std::endl << "✓ 消息已送达: " << json_string_value(msg_obj) << std::endl;
                    }
                    break;
                }
                case 4002: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << std::endl << "✗ 发送失败: " << json_string_value(msg_obj) << std::endl;
                    }
                    break;
                }
                // 接收消息处理
                case 4010: {
                    const char* sender = nullptr;
                    const char* message = nullptr;
                    
                    json_t *sender_obj = json_object_get(root, "account");
                    json_t *msg_obj = json_object_get(root, "message");
                    
                    if (json_is_string(sender_obj)) sender = json_string_value(sender_obj);
                    if (json_is_string(msg_obj)) message = json_string_value(msg_obj);
                    
                    if (sender && message) {
                        std::cout << std::endl << COLOR_BLUE << "[来自 " << sender << " 的消息] " << std::endl
                                << COLOR_RESET << message << std::endl;
                    } else {
                        std::cerr << std::endl << "收到格式错误的消息" << std::endl;
                    }
                    break;
                }
                case 5001:
                    user_info->logout();
                    std::cout << std::endl << "下线成功。" << std::endl;
                    break;
                default:
                    std::cout << std::endl << "[Server]: " << buffer << std::endl;
                    break;
            }
            json_decref(root);
        } else if (bytes_received == 0) {
            std::cout << std::endl << "# 服务器关闭了连接。" << std::endl;
            running = false;
            break;
        } else {
            perror("recv失败");
            running = false;
            break;
        }

        std::cout << CommandLinePrompt << std::flush;  // 保持输入提示可见
    }
}

int main() {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    std::vector<int> sockfds;
    for(int i=0;i<1;i++){
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


    for(int i=0;i<50000;i++){
        std::string account = std::to_string(i);
        std::string password = std::to_string(i);
        std::string username = std::to_string(i);
        std::string phone = std::to_string(i);
        std::string email = std::to_string(i) + "@gmail.com";

        json_t *root = json_object();
        if (root == nullptr) {
            std::cerr << "Failed to create JSON object" << std::endl;
            continue;
        }
        json_object_set_new(root, "type", json_integer(1000));
        json_object_set_new(root, "account", json_string(account.c_str()));
        json_object_set_new(root, "password", json_string(password.c_str()));
        json_object_set_new(root, "username", json_string(username.c_str()));
        json_object_set_new(root, "phone_number", json_string(phone.c_str()));
        json_object_set_new(root, "email", json_string(email.c_str()));
        
        char *json_str = json_dumps(root, JSON_COMPACT);
        if (json_str == nullptr) {
            std::cerr << "Failed to dump JSON object to string" << std::endl;
            json_decref(root);
            continue;
        }

        std::string json_str_with_delimiter = std::string(json_str) + "\r\n";
        std::cout << "Sending JSON: " << json_str << std::endl;

        if(send(sockfds[0], json_str_with_delimiter.c_str(), json_str_with_delimiter.size(), 0) == -1){
            perror("send error");
        }
        free(json_str);
        json_decref(root);
    }

    //UserInfo user_info;
    //std::thread sender(sendMessage, sockfd, &user_info);
    //std::thread receiver(receiveMessage, sockfd, &user_info);
    //sender.join();
    //receiver.join();

    sleep(10000);

    //close(sockfd);
    return 0;
}