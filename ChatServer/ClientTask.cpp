#include "ClientTask.hpp"
// 密码加密处理方法的定义
std::string generateSalt(size_t length) {
    unsigned char salt[length];
    if (RAND_bytes(salt, length) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    return std::string(reinterpret_cast<char*>(salt), length);
}

std::string hashPassword(const std::string& password, const std::string& salt, int iterations, size_t key_len) {
    unsigned char hash[key_len];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                        reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                        iterations, EVP_sha256(), key_len, hash) != 1) {
        throw std::runtime_error("Failed to hash password");
    }
    return std::string(reinterpret_cast<char*>(hash), key_len);
}

std::string toHex(const std::string& input) {
    std::ostringstream oss;
    for (unsigned char c : input) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string fromHex(const std::string& input) {
    std::string output;
    if (input.length() % 2 != 0) {
        throw std::invalid_argument("Invalid hex string");
    }
    for (size_t i = 0; i < input.length(); i += 2) {
        std::string byteStr = input.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteStr, nullptr, 16));
        output.push_back(byte);
    }
    return output;
}

// ClientTask 类定义

ClientTask::ClientTask(int clientFd, int epollFd, DBOperation* dbopPtr, LRUTokenManager* LRUm)
    : _clientFd(clientFd), _epollFd(epollFd), _dbopPtr(dbopPtr), _LRUm(LRUm)
{}

void ClientTask::execute() {
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

    // 打印接收到的原始 JSON 内容
    //debug("Received raw JSON: %s", buffer);
    // 将解析后的 JSON 对象序列化为字符串并打印
    char* json_str = json_dumps(root, JSON_INDENT(2));
    if (json_str) {
        debug("Parsed JSON: %s", json_str);
        free(json_str);
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
    handleMessageType(type, root);
    json_decref(root);
}

void ClientTask::handleMessageType(int type, json_t *root) {
    switch (type) {
        case 1000:
            handleType1(root);
            break;
        case 2000:
            handleType2(root);
            break;
        case 3000:
            handleType3(root);
            break;
        case 4000:
            handleType4(root);
            break;
        case 5000:
            handleType5(root);
            break;
        default:
            std::cerr << "Unknown message type: " << type << std::endl;
            break;
    }
}

ssize_t ClientTask::sendAll(int sockfd, const char* message) {
    size_t dataLen = strlen(message);  // 数据总长度
    size_t totalSent = 0;  // 已发送字节数

    while (totalSent < dataLen) {
        ssize_t sent = send(sockfd, message + totalSent, dataLen - totalSent, 0);
        
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 如果发生缓冲区满的情况，稍等后重试
                usleep(1000);  // 延迟 1 毫秒再尝试发送
                continue;  // 继续发送剩余的数据
            } else {
                perror("send failed");
                return -1;  // 发送失败，返回 -1
            }
        } else if (sent == 0) {
            fprintf(stderr, "Connection closed by peer\n");
            return -1;  // 连接关闭，返回 -1
        }

        totalSent += sent;  // 累加已发送的字节数
    }

    return totalSent;  // 返回已发送的字节数
}

ssize_t ClientTask::sendResult(int sockfd, int type, const char* message) {
    // 使用 jansson 创建 JSON 对象
    json_t *response = json_object();  // jansson 的创建对象函数

    // 设置 type 字段
    json_object_set_new(response, "type", json_integer(type));  // jansson 的函数

    // 设置 message 字段
    json_object_set_new(response, "message", json_string(message));

    // 获取序列化后的 JSON 字符串
    char* responseStr = json_dumps(response, JSON_COMPACT);

    size_t dataLen = strlen(responseStr);  // 数据总长度
    size_t totalSent = 0;  // 已发送字节数

    while (totalSent < dataLen) {
        ssize_t sent = send(sockfd, responseStr + totalSent, dataLen - totalSent, 0);

        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 如果发生缓冲区满的情况，稍等后重试
                usleep(1000);  // 延迟 1 毫秒再尝试发送
                continue;  // 继续发送剩余的数据
            } else {
                perror("send failed");
                free(responseStr);
                json_decref(response);  // 释放 JSON 对象
                return -1;  // 发送失败，返回 -1
            }
        } else if (sent == 0) {
            fprintf(stderr, "Connection closed by peer\n");
            free(responseStr);
            json_decref(response);  // 释放 JSON 对象
            return -1;  // 连接关闭，返回 -1
        }

        totalSent += sent;  // 累加已发送的字节数
    }
    free(responseStr);
    json_decref(response);  // 释放 JSON 对象
    return totalSent;  // 返回已发送的字节数
}

int ClientTask::sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users) {
    // 创建一个 JSON 对象，包含 "type" 和 "users" 字段
    json_t *response = json_object();
    
    // 设置 type 字段为 3001
    json_object_set_new(response, "type", json_integer(3001));

    // 创建一个 JSON 数组用于存储用户信息
    json_t *userArray = json_array();

    // 遍历用户信息（name, account）并构建对应的 JSON 对象
    for (const auto& user : users) {
        // 为每个用户创建一个 JSON 对象
        json_t *userObj = json_object();
        json_object_set_new(userObj, "name", json_string(user.first.c_str()));
        json_object_set_new(userObj, "account", json_string(user.second.c_str()));

        // 将该用户的 JSON 对象添加到数组中
        json_array_append_new(userArray, userObj);
    }

    // 将用户数组添加到 response 对象中的 "users" 字段
    json_object_set_new(response, "users", userArray);

    // 将 JSON 对象序列化为字符串
    char *responseStr = json_dumps(response, 0);

    if (responseStr) {
        // 使用 sendAll 发送 JSON 数据
        ssize_t sentBytes = sendAll(clientFd, responseStr);
        free(responseStr);  // 释放序列化后的 JSON 字符串
        json_decref(response);  // 释放 JSON 对象的内存

        // 判断发送字节数是否大于 0，返回成功或失败
        return (sentBytes > 0) ? 0 : -1;
    }

    json_decref(response);  // 释放 JSON 对象的内存
    return -1;  // 序列化失败，返回失败
}

void ClientTask::handleType1(json_t *root) {
    // 解析 account 字段
    json_t *account_json = json_object_get(root, "account");
    if (!json_is_string(account_json)) {
        std::cerr << "Invalid 'account' field in JSON" << std::endl;
        sendResult(_clientFd, 1002, "Invalid 'account'");
        return;
    }
    const char *account = json_string_value(account_json);
    // 解析 password 字段
    json_t *password_json = json_object_get(root, "password");
    if (!json_is_string(password_json)) {
        std::cerr << "Invalid 'password' field in JSON" << std::endl;
        sendResult(_clientFd, 1002, "Invalid 'password'");
        return;
    }
    const char *password = json_string_value(password_json);
    // 解析 username 字段
    json_t *username_json = json_object_get(root, "username");
    if (!json_is_string(username_json)) {
        std::cerr << "Invalid 'username' field in JSON" << std::endl;
        sendResult(_clientFd, 1002, "Invalid 'username'");
        return;
    }
    const char *username = json_string_value(username_json);
    // 解析 phone_number 字段
    json_t *phone_number_json = json_object_get(root, "phone_number");
    if (!json_is_string(phone_number_json)) {
        std::cerr << "Invalid 'phone_number' field in JSON" << std::endl;
        sendResult(_clientFd, 1002, "Invalid 'phone_number'");
        return;
    }
    const char *phone_number = json_string_value(phone_number_json);
    // 解析 email 字段
    json_t *email_json = json_object_get(root, "email");
    if (!json_is_string(email_json)) {
        std::cerr << "Invalid 'email' field in JSON" << std::endl;
        sendResult(_clientFd, 1002, "Invalid 'email'");
        return;
    }
    const char *email = json_string_value(email_json);

    User user(account, password, username, phone_number, email);
    int res = _dbopPtr->addUser(user);
    if(res == 0)
        sendResult(_clientFd, 1001, "success");
    else 
        sendResult(_clientFd, 1002, "fail");
}

void ClientTask::handleType2(json_t *root) {
    // 解析 account 字段
    json_t *account_json = json_object_get(root, "account");
    if (!json_is_string(account_json)) {
        std::cerr << "Invalid 'account' field in JSON" << std::endl;
        sendResult(_clientFd, 2002, "Invalid 'account'");
        return;
    }
    const char *account = json_string_value(account_json);
    // 解析 password 字段
    json_t *password_json = json_object_get(root, "password");
    if (!json_is_string(password_json)) {
        std::cerr << "Invalid 'password' field in JSON" << std::endl;
        sendResult(_clientFd, 2002, "Invalid 'password'");
        return;
    }
    const char *password = json_string_value(password_json);

    bool res = _dbopPtr->verifyUser(account, password);

    if(res){
        std::string token = _LRUm->generate_token();
        sendResult(_clientFd, 2001, token.c_str());
        std::string username = _dbopPtr->getUsername(account); 
        _LRUm->saveToken(account, token, username, _clientFd);
        sendResult(_clientFd, 2011, username.c_str());

        //发送离线时接收的消息
        bool flag = true;
        std::vector<std::string> strs = _dbopPtr->getMessage(account);
        for(auto& str: strs){
            json_t *json_obj = json_object();
            json_object_set_new(json_obj, "type", json_integer(4010));
            json_object_set_new(json_obj, "account", json_string(account));
            json_object_set_new(json_obj, "message", json_string(str.c_str()));
            // 转换为字符串
            char *msg = json_dumps(json_obj, JSON_COMPACT);
            json_decref(json_obj); // 释放 JSON 对象
            if (!msg) {
                std::cerr << "Failed to create JSON string" << std::endl;
                flag = false;
                return;
            }
            // 发送消息
            if (sendAll(_clientFd, msg) == -1) {
                perror("T2 Failed to send result\n");
                flag = false;
            }
            free(msg); // 释放 JSON 字符串
        }
        if(flag){
            _dbopPtr->deleteMessage(account);
        }
    }
    else
        sendResult(_clientFd, 2002, "fail");
}

void ClientTask::handleType3(json_t *root){
    json_t *account_json = json_object_get(root, "account");
    if (!json_is_string(account_json)) {
        std::cerr << "Invalid 'account' field in JSON" << std::endl;
        sendResult(_clientFd, 3002, "Invalid 'account'");
        return;
    }
    const char *account = json_string_value(account_json);
    // 解析 token 字段
    json_t *token_json = json_object_get(root, "token");
    if (!json_is_string(token_json)) {
        std::cerr << "Invalid 'token' field in JSON" << std::endl;
        sendResult(_clientFd, 3002, "Invalid 'token'");
        return;
    }
    const char *token = json_string_value(token_json);
    // 验证 token
    if (!_LRUm->verifyToken(account, token)) {
        std::cerr << "Invalid or expired token" << std::endl;
        sendResult(_clientFd, 3002, "Invalid or expired token, try relog please.");
        return;
    }
    // 获取在线用户列表
    std::vector<std::pair<std::string, std::string>> userPairs = _LRUm->getAllUsers();
    sendUsersInfo(_clientFd, userPairs);
}

void ClientTask::handleType4(json_t *root){
    json_t *account_json = json_object_get(root, "account");
    if (!json_is_string(account_json)) {
        std::cerr << "Invalid 'account' field in JSON" << std::endl;
        sendResult(_clientFd, 4002, "Invalid 'account'");
        return;
    }
    const char *account = json_string_value(account_json);
    // 解析 token 字段
    json_t *token_json = json_object_get(root, "token");
    if (!json_is_string(token_json)) {
        std::cerr << "Invalid 'token' field in JSON" << std::endl;
        sendResult(_clientFd, 4002, "Invalid 'token'");
        return;
    }
    const char *token = json_string_value(token_json);
    // 验证 token
    if (!_LRUm->verifyToken(account, token)) {
        std::cerr << "Invalid or expired token" << std::endl;
        sendResult(_clientFd, 4002, "Invalid or expired token");
        return;
    }
    // 解析 receiver_useraccount 字段
    json_t *receiver_json = json_object_get(root, "receiver_useraccount");
    if (!json_is_string(receiver_json)) {
        std::cerr << "Invalid 'receiver_useraccount' field in JSON" << std::endl;
        sendResult(_clientFd, 4002, "Invalid 'receiver_useraccount'");
        return;
    }
    const char *receiver = json_string_value(receiver_json);
    // 解析 message 字段
    json_t *message_json = json_object_get(root, "message");
    if (!json_is_string(message_json)) {
        std::cerr << "Invalid 'message' field in JSON" << std::endl;
        sendResult(_clientFd, 4002, "Invalid 'message'");
        return;
    }
    const char *message = json_string_value(message_json);

    // 获取接收者的文件描述符
    int receiver_fd = _LRUm->getUserFd(receiver);
    if (receiver_fd == -1) {
        _dbopPtr->addMessage(account , receiver, message);
        return;
    }

    // 构造消息并发送
    // 创建 JSON 对象
    json_t *json_obj = json_object();
    json_object_set_new(json_obj, "type", json_integer(4010));
    json_object_set_new(json_obj, "account", json_string(account));
    json_object_set_new(json_obj, "message", json_string(message));
    // 转换为字符串
    char *msg = json_dumps(json_obj, JSON_COMPACT);
    json_decref(json_obj); // 释放 JSON 对象
    if (!msg) {
        std::cerr << "Failed to create JSON string" << std::endl;
        return;
    }
    // 发送消息
    if (sendAll(receiver_fd, msg) == -1) {
        _dbopPtr->addMessage(account, receiver, msg);
        perror("Failed to send result\n");
    }
    free(msg); // 释放 JSON 字符串
}

void ClientTask::handleType5(json_t *root){
    json_t *account_json = json_object_get(root, "account");
    if (!json_is_string(account_json)) {
        std::cerr << "Invalid 'account' field in JSON" << std::endl;
        sendResult(_clientFd, 5002, "Fail");
        return;
    }
    const char *account = json_string_value(account_json);
    json_t *token_json = json_object_get(root, "token");
    if (!json_is_string(token_json)) {
        std::cerr << "Invalid 'token' field in JSON" << std::endl;
        sendResult(_clientFd, 5002, "Fail");
        return;
    }
    const char *token = json_string_value(token_json);
    if (!_LRUm->verifyToken(account, token)) {
        std::cerr << "Invalid or expired token" << std::endl;
        sendResult(_clientFd, 5002, "Fail");
        return;
    }
    if (!_LRUm->logout(account)) {
        std::cerr << "Failed to log out user" << std::endl;
        sendResult(_clientFd, 5002, "Fail");
        return;
    }
    std::ostringstream oss;
    oss << "User " << account << " logged out successfully" << std::endl;
    info_str(oss.str());

    sendResult(_clientFd, 5001, "success");
    freeFd();
}

void ClientTask::freeFd(){
    epoll_event event;
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _clientFd, &event) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL");
    }
    close(_clientFd);
}