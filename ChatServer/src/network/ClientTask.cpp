#include "ClientTask.hpp"


// ClientTask 类定义

ClientTask::ClientTask(int clientFd, int epollFd, DBOperation* dbopPtr, LRUTokenManager* LRUm, std::set<int>* ss)
    : _clientFd(clientFd), _epollFd(epollFd), _dbopPtr(dbopPtr), _LRUm(LRUm), _ss(ss)
{}

bool ClientTask::recvAll(int sockfd, void* buffer, size_t len){
    size_t totalReceived = 0;
    char* buf = (char*)buffer;
    while(totalReceived < len){
        ssize_t received = recv(sockfd, buf + totalReceived, len - totalReceived, 0);
        if(received <= 0) return false;
        totalReceived += received;
    }
    return true;
}

void ClientTask::execute() {
    uint32_t key;
    while(recvAll(_clientFd, &key, sizeof(key))) {
        key = ntohl(key); 

        switch(key){
            case PROTOBUF_KEY:
                PROTOBUF_handle();
                break; 
            // 拓展其他字节流
        }
    }
    _ss->erase(_clientFd);
}


void ClientTask::PROTOBUF_handle(){
    uint32_t length;
    if (!recvAll(_clientFd, &length, sizeof(length))) return;

    length = ntohl(length);
    debug_str("收到报文长度" + std::to_string(length));

    //读取
    std::string received_data(length, '\0');
    if (!recvAll(_clientFd, &received_data[0], length)) return;
    debug_str("protobuf: " + received_data.substr(0,50)); // 防止日志过载

    chat::ChatMessage msg; // 只用一下type字段
    if (!msg.ParseFromString(received_data)) {
        fatal_str("Failed to parse protobuf message!");
        return;
    }

    PROTOBUF_handleMessageType(msg.type(),received_data); 
}


ssize_t ClientTask::sendAll(int sockfd, const char* data, size_t len) {
    size_t totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = send(sockfd, data + totalSent, len - totalSent, 0);
        
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);  // 发送缓冲区满时，等待 1ms 再尝试
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

ssize_t ClientTask::PROTOBUF_sendAll(int sockfd, std::string serialized_data){// KLV打包
    uint32_t key = htonl(PROTOBUF_KEY);
    uint32_t length = htonl(serialized_data.size());
    std::string packet;
    packet.append(reinterpret_cast<const char*>(&key), sizeof(key));  // K
    packet.append(reinterpret_cast<const char*>(&length), sizeof(length));  // L
    packet.append(serialized_data);  // V（Protobuf 数据）
    return sendAll(sockfd, packet.data(), packet.size());
}

ssize_t ClientTask::PROTOBUF_sendResult(int sockfd, int type, const char* message){
    chat::Response msg;
    msg.set_type(type);
    msg.set_message(message);

    //序列化
    std::string serialized_data;
    if(!msg.SerializeToString(&serialized_data)){
        fatal_str("Protobuf 序列化失败！");
        return -1;
    }
    return PROTOBUF_sendAll(sockfd, serialized_data);
}
ssize_t ClientTask::PROTOBUF_sendLoginResult(int sockfd, std::string& username, std::string& token){
    chat::LoginResponse msg;
    msg.set_type(2001);
    msg.set_username(username);
    msg.set_token(token);

    //序列化
    std::string serialized_data;
    if(!msg.SerializeToString(&serialized_data)){
        fatal_str("Protobuf 序列化失败！");
        return -1;
    }
    return PROTOBUF_sendAll(sockfd, serialized_data);
}
ssize_t ClientTask::PROTOBUF_sendUsersInfo(int sockfd, const std::vector<std::pair<std::string, std::string>>& users) {
    chat::GetOnlineUsersResponse response;
    response.set_type(3001); 

    for(auto&x:users){
        chat::UserInfo* user1 = response.add_users();
        user1->set_name(x.first);
        user1->set_account(x.second);
    }

    std::string serialized_data;
    if(!response.SerializeToString(&serialized_data)){
        fatal_str("Protobuf 序列化失败！");
        return -1;
    }
    return PROTOBUF_sendAll(sockfd, serialized_data);
}
ssize_t ClientTask::PROTOBUF_sendChatMessage(int sockfd, const std::string& account, const std::string& message){
    chat::ReceivedMessage response;
    response.set_type(4010);
    response.set_account(account);
    response.set_message(message);

    std::string serialized_data;
    if(!response.SerializeToString(&serialized_data)){
        fatal_str("Protobuf 序列化失败！");
        return -1;
    }
    return PROTOBUF_sendAll(sockfd, serialized_data);
}

void ClientTask::PROTOBUF_handleMessageType(int type, const std::string& msg) {
    switch (type) {
        case 1000:
            PROTOBUF_handleType1(msg);
            break;
        case 2000:
            PROTOBUF_handleType2(msg);
            break;
        case 3000:
            PROTOBUF_handleType3(msg);
            break;
        case 4000:
            PROTOBUF_handleType4(msg);
            break;
        case 5000:
            PROTOBUF_handleType5(msg);
            break;
        default:
            std::ostringstream oss;
            oss << "Unknown message type: " << type;
            fatal_str(oss.str());
            break;
    }
}
void ClientTask::PROTOBUF_handleType1(const std::string& received_data) {
    chat::RegisterRequest msg;
    if (!msg.ParseFromString(received_data)) {
        std::cerr << "Failed to parse base message!" << std::endl;
        return;
    }

    User user(msg.account(), msg.password(), msg.username(), msg.phone_number(), msg.email());
    int res = _dbopPtr->addUser(user);
    if(res == 0)
        PROTOBUF_sendResult(_clientFd, 1001, "success");
    else 
        PROTOBUF_sendResult(_clientFd, 1002, "fail");
}
void ClientTask::PROTOBUF_handleType2(const std::string& received_data) {
    chat::LoginRequest msg;
    if (!msg.ParseFromString(received_data)) {
        std::cerr << "Failed to parse base message!" << std::endl;
        PROTOBUF_sendResult(_clientFd, 2002, "login error");
        return;
    }
    std::string account = msg.account();
    bool res = _dbopPtr->verifyUser(account, msg.password());
    if(res){
        //登录成功，获取token
        std::string username = _dbopPtr->getUsername(account); 
        std::string token = _LRUm->generate_token();
        _LRUm->saveToken(account, token, username, _clientFd);
        PROTOBUF_sendLoginResult(_clientFd, username, token);

        //发送离线时接收的消息
        bool flag = true;
        std::vector<std::string> strs = _dbopPtr->getMessage(account);
        for(auto& str: strs){
            // 发送消息
            if (PROTOBUF_sendChatMessage(_clientFd,account ,str) == -1) {
                perror("Failed to send history msg\n");
                flag = false;
            }
        }
        if(flag){
            _dbopPtr->deleteMessage(account);
        }
    }
    else
        PROTOBUF_sendResult(_clientFd, 2002, "fail");
}
void ClientTask::PROTOBUF_handleType3(const std::string& received_data){
    chat::GetOnlineUsersRequest msg;
    if (!msg.ParseFromString(received_data)) {
        std::cerr << "Failed to parse base message!" << std::endl;
        return;
    }

    // 验证 token
    if (!_LRUm->verifyToken(msg.account(), msg.token())) {
        std::cerr << "Invalid or expired token" << std::endl;
        PROTOBUF_sendResult(_clientFd, 3002, "Invalid or expired token, try relog please.");
        return;
    }
    // 获取在线用户列表
    std::vector<std::pair<std::string, std::string>> userPairs = _LRUm->getAllUsers();
    debug_str("current user number: " + std::to_string(userPairs.size()));
    PROTOBUF_sendUsersInfo(_clientFd, userPairs);
}
void ClientTask::PROTOBUF_handleType4(const std::string& received_data){
    chat::ChatMessage msg;
    if (!msg.ParseFromString(received_data)) {
        std::cerr << "Failed to parse base message!" << std::endl;
        return;
    }
    std::string account = msg.account();
    // 验证 token
    if (!_LRUm->verifyToken(account, msg.token())) {
        std::cerr << "Invalid or expired token" << std::endl;
        PROTOBUF_sendResult(_clientFd, 4002, "Invalid or expired token");
        return;
    }

    PROTOBUF_sendResult(_clientFd, 4001, "chat message sent success");

    // 获取接收者的文件描述符
    std::string receiver = msg.receiver_useraccount();
    int receiver_fd = _LRUm->getUserFd(receiver);
    if (receiver_fd == -1) { // 不在线
        _dbopPtr->addMessage(account , receiver, msg.message());
        perror("Failed to send result\n");
    }
    else if (PROTOBUF_sendChatMessage(receiver_fd, account, msg.message()) == -1) {
        _dbopPtr->addMessage(account, receiver, msg.message());
        perror("Failed to send result\n");
    }
}
void ClientTask::PROTOBUF_handleType5(const std::string& received_data){
    chat::LogoutRequest msg;
    if (!msg.ParseFromString(received_data)) {
        std::cerr << "Failed to parse base message!" << std::endl;
        return;
    }
    std::string account = msg.account();
    if (!_LRUm->verifyToken(account, msg.token())) {
        std::cerr << "Invalid or expired token" << std::endl;
        PROTOBUF_sendResult(_clientFd, 5002, "Invalid or expired token");
        return;
    }
    if (!_LRUm->logout(account)) {
        std::cerr << "user may had logged out" << std::endl;
        PROTOBUF_sendResult(_clientFd, 5002, "user may had logged out");
        return;
    }
    std::ostringstream oss;
    oss << "User " << account << " logged out successfully" << std::endl;
    info_str(oss.str());

    PROTOBUF_sendResult(_clientFd, 5001, "success");
    freeFd();
}



// JSON
// ssize_t ClientTask::JSON_sendResult(int sockfd, int type, const char* message) {
//     // 使用 jansson 创建 JSON 对象
//     json_t *response = json_object();  // jansson 的创建对象函数

//     // 设置 type 字段
//     json_object_set_new(response, "type", json_integer(type));  // jansson 的函数

//     // 设置 message 字段
//     json_object_set_new(response, "message", json_string(message));

//     // 获取序列化后的 JSON 字符串
//     char* responseStr = json_dumps(response, JSON_COMPACT);

//     size_t dataLen = strlen(responseStr);  // 数据总长度
//     size_t totalSent = 0;  // 已发送字节数

//     while (totalSent < dataLen) {
//         ssize_t sent = send(sockfd, responseStr + totalSent, dataLen - totalSent, 0);

//         if (sent == -1) {
//             if (errno == EAGAIN || errno == EWOULDBLOCK) {
//                 // 如果发生缓冲区满的情况，稍等后重试
//                 usleep(1000);  // 延迟 1 毫秒再尝试发送
//                 continue;  // 继续发送剩余的数据
//             } else {
//                 perror("send failed");
//                 free(responseStr);
//                 json_decref(response);  // 释放 JSON 对象
//                 return -1;  // 发送失败，返回 -1
//             }
//         } else if (sent == 0) {
//             fprintf(stderr, "Connection closed by peer\n");
//             free(responseStr);
//             json_decref(response);  // 释放 JSON 对象
//             return -1;  // 连接关闭，返回 -1
//         }

//         totalSent += sent;  // 累加已发送的字节数
//     }
//     free(responseStr);
//     json_decref(response);  // 释放 JSON 对象
//     return totalSent;  // 返回已发送的字节数
// }
// int ClientTask::JSON_sendUsersInfo(int clientFd, const std::vector<std::pair<std::string, std::string>>& users) {
//     // 创建一个 JSON 对象，包含 "type" 和 "users" 字段
//     json_t *response = json_object();
    
//     // 设置 type 字段为 3001
//     json_object_set_new(response, "type", json_integer(3001));

//     // 创建一个 JSON 数组用于存储用户信息
//     json_t *userArray = json_array();

//     // 遍历用户信息（name, account）并构建对应的 JSON 对象
//     for (const auto& user : users) {
//         // 为每个用户创建一个 JSON 对象
//         json_t *userObj = json_object();
//         json_object_set_new(userObj, "name", json_string(user.first.c_str()));
//         json_object_set_new(userObj, "account", json_string(user.second.c_str()));

//         // 将该用户的 JSON 对象添加到数组中
//         json_array_append_new(userArray, userObj);
//     }

//     // 将用户数组添加到 response 对象中的 "users" 字段
//     json_object_set_new(response, "users", userArray);

//     // 将 JSON 对象序列化为字符串
//     char *responseStr = json_dumps(response, 0);

//     if (responseStr) {
//         // 使用 sendAll 发送 JSON 数据
//         ssize_t sentBytes = sendAll(clientFd, responseStr);
//         free(responseStr);  // 释放序列化后的 JSON 字符串
//         json_decref(response);  // 释放 JSON 对象的内存

//         // 判断发送字节数是否大于 0，返回成功或失败
//         return (sentBytes > 0) ? 0 : -1;
//     }

//     json_decref(response);  // 释放 JSON 对象的内存
//     return -1;  // 序列化失败，返回失败
// }
// void ClientTask::JSON_handleMessageType(int type, json_t *root) {
//     switch (type) {
//         case 1000:
//             JSON_handleType1(root);
//             break;
//         case 2000:
//             JSON_handleType2(root);
//             break;
//         case 3000:
//             JSON_handleType3(root);
//             break;
//         case 4000:
//             JSON_handleType4(root);
//             break;
//         case 5000:
//             JSON_handleType5(root);
//             break;
//         default:
//             std::cerr << "Unknown message type: " << type << std::endl;
//             break;
//     }
// }
// void ClientTask::JSON_handleType1(json_t *root) {
//     // 解析 account 字段
//     json_t *account_json = json_object_get(root, "account");
//     if (!json_is_string(account_json)) {
//         std::cerr << "Invalid 'account' field in JSON" << std::endl;
//         sendResult(_clientFd, 1002, "Invalid 'account'");
//         return;
//     }
//     const char *account = json_string_value(account_json);
//     // 解析 password 字段
//     json_t *password_json = json_object_get(root, "password");
//     if (!json_is_string(password_json)) {
//         std::cerr << "Invalid 'password' field in JSON" << std::endl;
//         sendResult(_clientFd, 1002, "Invalid 'password'");
//         return;
//     }
//     const char *password = json_string_value(password_json);
//     // 解析 username 字段
//     json_t *username_json = json_object_get(root, "username");
//     if (!json_is_string(username_json)) {
//         std::cerr << "Invalid 'username' field in JSON" << std::endl;
//         sendResult(_clientFd, 1002, "Invalid 'username'");
//         return;
//     }
//     const char *username = json_string_value(username_json);
//     // 解析 phone_number 字段
//     json_t *phone_number_json = json_object_get(root, "phone_number");
//     if (!json_is_string(phone_number_json)) {
//         std::cerr << "Invalid 'phone_number' field in JSON" << std::endl;
//         sendResult(_clientFd, 1002, "Invalid 'phone_number'");
//         return;
//     }
//     const char *phone_number = json_string_value(phone_number_json);
//     // 解析 email 字段
//     json_t *email_json = json_object_get(root, "email");
//     if (!json_is_string(email_json)) {
//         std::cerr << "Invalid 'email' field in JSON" << std::endl;
//         sendResult(_clientFd, 1002, "Invalid 'email'");
//         return;
//     }
//     const char *email = json_string_value(email_json);

//     User user(account, password, username, phone_number, email);
//     int res = _dbopPtr->addUser(user);
//     if(res == 0)
//         sendResult(_clientFd, 1001, "success");
//     else 
//         sendResult(_clientFd, 1002, "fail");
// }
// void ClientTask::JSON_handleType2(json_t *root) {
//     // 解析 account 字段
//     json_t *account_json = json_object_get(root, "account");
//     if (!json_is_string(account_json)) {
//         std::cerr << "Invalid 'account' field in JSON" << std::endl;
//         sendResult(_clientFd, 2002, "Invalid 'account'");
//         return;
//     }
//     const char *account = json_string_value(account_json);
//     // 解析 password 字段
//     json_t *password_json = json_object_get(root, "password");
//     if (!json_is_string(password_json)) {
//         std::cerr << "Invalid 'password' field in JSON" << std::endl;
//         sendResult(_clientFd, 2002, "Invalid 'password'");
//         return;
//     }
//     const char *password = json_string_value(password_json);

//     bool res = _dbopPtr->verifyUser(account, password);

//     if(res){
//         std::string token = _LRUm->generate_token();
//         sendResult(_clientFd, 2001, token.c_str());
//         std::string username = _dbopPtr->getUsername(account); 
//         _LRUm->saveToken(account, token, username, _clientFd);
//         sendResult(_clientFd, 2011, username.c_str());

//         //发送离线时接收的消息
//         bool flag = true;
//         std::vector<std::string> strs = _dbopPtr->getMessage(account);
//         for(auto& str: strs){
//             json_t *json_obj = json_object();
//             json_object_set_new(json_obj, "type", json_integer(4010));
//             json_object_set_new(json_obj, "account", json_string(account));
//             json_object_set_new(json_obj, "message", json_string(str.c_str()));
//             // 转换为字符串
//             char *msg = json_dumps(json_obj, JSON_COMPACT);
//             json_decref(json_obj); // 释放 JSON 对象
//             if (!msg) {
//                 std::cerr << "Failed to create JSON string" << std::endl;
//                 flag = false;
//                 return;
//             }
//             // 发送消息
//             if (sendAll(_clientFd, msg) == -1) {
//                 perror("T2 Failed to send result\n");
//                 flag = false;
//             }
//             free(msg); // 释放 JSON 字符串
//         }
//         if(flag){
//             _dbopPtr->deleteMessage(account);
//         }
//     }
//     else
//         sendResult(_clientFd, 2002, "fail");
// }
// void ClientTask::JSON_handleType3(json_t *root){
//     json_t *account_json = json_object_get(root, "account");
//     if (!json_is_string(account_json)) {
//         std::cerr << "Invalid 'account' field in JSON" << std::endl;
//         sendResult(_clientFd, 3002, "Invalid 'account'");
//         return;
//     }
//     const char *account = json_string_value(account_json);
//     // 解析 token 字段
//     json_t *token_json = json_object_get(root, "token");
//     if (!json_is_string(token_json)) {
//         std::cerr << "Invalid 'token' field in JSON" << std::endl;
//         sendResult(_clientFd, 3002, "Invalid 'token'");
//         return;
//     }
//     const char *token = json_string_value(token_json);
//     // 验证 token
//     if (!_LRUm->verifyToken(account, token)) {
//         std::cerr << "Invalid or expired token" << std::endl;
//         sendResult(_clientFd, 3002, "Invalid or expired token, try relog please.");
//         return;
//     }
//     // 获取在线用户列表
//     std::vector<std::pair<std::string, std::string>> userPairs = _LRUm->getAllUsers();
//     sendUsersInfo(_clientFd, userPairs);
// }
// void ClientTask::JSON_handleType4(json_t *root){
//     json_t *account_json = json_object_get(root, "account");
//     if (!json_is_string(account_json)) {
//         std::cerr << "Invalid 'account' field in JSON" << std::endl;
//         sendResult(_clientFd, 4002, "Invalid 'account'");
//         return;
//     }
//     const char *account = json_string_value(account_json);
//     // 解析 token 字段
//     json_t *token_json = json_object_get(root, "token");
//     if (!json_is_string(token_json)) {
//         std::cerr << "Invalid 'token' field in JSON" << std::endl;
//         sendResult(_clientFd, 4002, "Invalid 'token'");
//         return;
//     }
//     const char *token = json_string_value(token_json);
//     // 验证 token
//     if (!_LRUm->verifyToken(account, token)) {
//         std::cerr << "Invalid or expired token" << std::endl;
//         sendResult(_clientFd, 4002, "Invalid or expired token");
//         return;
//     }
//     // 解析 receiver_useraccount 字段
//     json_t *receiver_json = json_object_get(root, "receiver_useraccount");
//     if (!json_is_string(receiver_json)) {
//         std::cerr << "Invalid 'receiver_useraccount' field in JSON" << std::endl;
//         sendResult(_clientFd, 4002, "Invalid 'receiver_useraccount'");
//         return;
//     }
//     const char *receiver = json_string_value(receiver_json);
//     // 解析 message 字段
//     json_t *message_json = json_object_get(root, "message");
//     if (!json_is_string(message_json)) {
//         std::cerr << "Invalid 'message' field in JSON" << std::endl;
//         sendResult(_clientFd, 4002, "Invalid 'message'");
//         return;
//     }
//     const char *message = json_string_value(message_json);

//     // 获取接收者的文件描述符
//     int receiver_fd = _LRUm->getUserFd(receiver);
//     if (receiver_fd == -1) {
//         _dbopPtr->addMessage(account , receiver, message);
//         return;
//     }

//     // 构造消息并发送
//     // 创建 JSON 对象
//     json_t *json_obj = json_object();
//     json_object_set_new(json_obj, "type", json_integer(4010));
//     json_object_set_new(json_obj, "account", json_string(account));
//     json_object_set_new(json_obj, "message", json_string(message));
//     // 转换为字符串
//     char *msg = json_dumps(json_obj, JSON_COMPACT);
//     json_decref(json_obj); // 释放 JSON 对象
//     if (!msg) {
//         std::cerr << "Failed to create JSON string" << std::endl;
//         return;
//     }
//     // 发送消息
//     if (sendAll(receiver_fd, msg) == -1) {
//         _dbopPtr->addMessage(account, receiver, msg);
//         perror("Failed to send result\n");
//     }
//     free(msg); // 释放 JSON 字符串
// }
// void ClientTask::JSON_handleType5(json_t *root){
//     json_t *account_json = json_object_get(root, "account");
//     if (!json_is_string(account_json)) {
//         std::cerr << "Invalid 'account' field in JSON" << std::endl;
//         sendResult(_clientFd, 5002, "Fail");
//         return;
//     }
//     const char *account = json_string_value(account_json);
//     json_t *token_json = json_object_get(root, "token");
//     if (!json_is_string(token_json)) {
//         std::cerr << "Invalid 'token' field in JSON" << std::endl;
//         sendResult(_clientFd, 5002, "Fail");
//         return;
//     }
//     const char *token = json_string_value(token_json);
//     if (!_LRUm->verifyToken(account, token)) {
//         std::cerr << "Invalid or expired token" << std::endl;
//         sendResult(_clientFd, 5002, "Fail");
//         return;
//     }
//     if (!_LRUm->logout(account)) {
//         std::cerr << "Failed to log out user" << std::endl;
//         sendResult(_clientFd, 5002, "Fail");
//         return;
//     }
//     std::ostringstream oss;
//     oss << "User " << account << " logged out successfully" << std::endl;
//     info_str(oss.str());

//     sendResult(_clientFd, 5001, "success");
//     freeFd();
// }

//
void ClientTask::freeFd(){
    epoll_event event;
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _clientFd, &event) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL");
    }
    close(_clientFd);
}