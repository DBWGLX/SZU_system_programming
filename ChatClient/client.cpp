#include <iostream>
#include <cstdio>
#include <thread>
#include <atomic>
#include <cstring>
#include <mutex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jansson.h>

#define SERVER_PORT 8080

// 颜色定义用于更好地区分消息类型
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_RESET  "\033[0m"

#define CommandLinePrompt "\033[32m[ChatClient]$\033[0m "

std::atomic<bool> running(true);

struct UserInfo {
    std::mutex mutex;
    std::string account;
    std::string token;
    bool is_logged_in = false;

    void setToken(const std::string &acc, const std::string &tkn) {
        std::lock_guard<std::mutex> lock(mutex);
        account = acc;
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

void sendMessage(int sockfd, UserInfo *user_info) {
    printf("\n# 发送线程已启动\n");
    printf("输入'/help'以获取帮助\n");

    while (running) {
        printf(CommandLinePrompt);
        std::string command;
        std::getline(std::cin, command);

        if (command.empty()) continue;

        if (command == "/quit") {
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
        } else if (command == "/register") {
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
        } else if (command == "/login") {
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
        } else if (command == "/online") {
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
        } else if (command == "/send") {
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
        } else if (command == "/help"){
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

void receiveMessage(int sockfd, UserInfo *user_info) {
    printf("\n# 接收线程已启动\n");
    char buffer[1024];
    while (running) {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        printf("\n⭐debug: 收到请求了\n");
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
                        std::cout << COLOR_GREEN << "注册成功: " << json_string_value(msg_obj) 
                            << COLOR_RESET << std::endl;
                    }
                    break;
                }
                case 1002: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << "注册失败: " << json_string_value(msg_obj) << std::endl;
                    }
                    break;
                }
                case 2001: { // 登录成功
                    json_t *token_obj = json_object_get(root, "message");
                    if (json_is_string(token_obj)) {
                        const char *token = json_string_value(token_obj);
                        std::string account;
                        {
                            std::lock_guard<std::mutex> lock(user_info->mutex);
                            account = user_info->account;
                        }
                        user_info->setToken(account, token);
                        std::cout << "登录成功，token已保存。" << std::endl;
                    }
                    break;
                }
                case 2002: // 登录失败
                    user_info->logout();
                    std::cout << "登录失败。" << std::endl;
                    break;
                case 3001: { // 在线用户列表
                    json_t *users = json_object_get(root, "users");
                    if (json_is_array(users)) {
                        std::cout << "在线用户列表：" << std::endl;
                        size_t index;
                        json_t *value;
                        json_array_foreach(users, index, value) {
                            const char *name = json_string_value(json_object_get(value, "name"));
                            const char *account = json_string_value(json_object_get(value, "account"));
                            if (name && account) {
                                std::cout << "姓名: " << name << ", 账号: " << account << std::endl;
                            }
                        }
                    }
                    break;
                }
                   // 消息发送状态处理
                case 4001: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << "✓ 消息已送达: " << json_string_value(msg_obj) << std::endl;
                    }
                    break;
                }
                case 4002: {
                    json_t *msg_obj = json_object_get(root, "message");
                    if (json_is_string(msg_obj)) {
                        std::cout << "✗ 发送失败: " << json_string_value(msg_obj) << std::endl;
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
                        std::cout << COLOR_BLUE << "\n[来自 " << sender << " 的消息] " 
                                << COLOR_RESET << message << std::endl;
                        std::cout << CommandLinePrompt << std::flush;  // 保持输入提示可见
                    } else {
                        std::cerr << "收到格式错误的消息" << std::endl;
                    }
                    break;
                }
                case 5001: // 下线成功
                    user_info->logout();
                    std::cout << "下线成功。" << std::endl;
                    break;
                default:
                    std::cout << "[Server]: " << buffer << std::endl;
                    break;
            }
            json_decref(root);
        } else if (bytes_received == 0) {
            std::cout << "服务器关闭了连接。" << std::endl;
            running = false;
            break;
        } else {
            perror("recv失败");
            running = false;
            break;
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket创建失败");
        return 1;
    }
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        perror("连接失败");
        close(sockfd);
        return 1;
    }

    printf("已连接服务器\n");

    UserInfo user_info;
    std::thread sender(sendMessage, sockfd, &user_info);
    std::thread receiver(receiveMessage, sockfd, &user_info);

    sender.join();
    receiver.join();

    close(sockfd);
    return 0;
}