#include "ClientTask.hpp"


// ClientTask 类定义

ClientTask::ClientTask(int clientFd, std::string msg,int epollFd, LRUTokenManager* LRUm, io_uring* ring)
    : _clientFd(clientFd), _msg(msg), _epollFd(epollFd), _LRUm(LRUm), _ring(ring)
{   
    timeoutSeconds = std::chrono::seconds(2);
}

//执行逻辑
void ClientTask::execute(DBOperation& dbop) {
    _dbopPtr = &dbop;

    uint32_t key;
    std::memcpy(&key, &_msg[0], 4);
    key = ntohl(key); 
    switch(key){
        case PROTOBUF_KEY:
            PROTOBUF_handle();
            break; 
        // 拓展其他字节流处理协议
    }
}

void ClientTask::PROTOBUF_handle(){
    uint32_t length;
    std::memcpy(&length, &_msg[4], 4);
    length = ntohl(length);


    debug_str("收到报文长度" + std::to_string(length));
    debug_str("protobuf: " + _msg.substr(0,50)); // 只展示部分信息，防止日志过载

    chat::ChatMessage msg; // 只用一下type字段
    if (!msg.ParseFromString(_msg.substr(8))) {
        fatal_str("Failed to parse protobuf message!");
        return;
    }

    PROTOBUF_handleMessageType(msg.type(),_msg.substr(8)); 
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


// io_uring
size_t ClientTask::submitSend(SendContext* ctx) {
    size_t remaining = ctx->data.size() - ctx->offset;
    if (remaining == 0) {
        delete ctx; // 发送完毕，释放资源
        return 0;
    }

    io_uring_sqe* sqe = io_uring_get_sqe(_ring);
    if (!sqe) {
        fatal_str("❌ 获取SQE失败！");
        return -1;
    }

    // 发送时的指针
    void* ptr = (void*)(ctx->data.data() + ctx->offset);
    size_t len = remaining;

    io_uring_prep_send(sqe, ctx->sockfd, ptr, len, 0);
    io_uring_sqe_set_data(sqe, ctx); // 传回这个 ctx

    if (io_uring_submit(_ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        delete ctx;
        return -1;
    }
    return 0;
}

// 线程阻塞发
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


size_t ClientTask::PROTOBUF_sendAll(int sockfd, const std::string& serialized_data) {
    std::string msg;

    // 打包成 KLV 格式
    uint32_t key = htonl(PROTOBUF_KEY);
    uint32_t len = htonl(serialized_data.size());

    msg.append(reinterpret_cast<char*>(&key), sizeof(key));
    msg.append(reinterpret_cast<char*>(&len), sizeof(len));
    msg.append(serialized_data);

    //SendContext* ctx = new SendContext(sockfd, msg);
    //return submitSend(ctx);  // 发起第一次发送

    return sendAll(sockfd, msg.data(), msg.size());
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

//
void ClientTask::freeFd(){
    epoll_event event;
    if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, _clientFd, &event) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL");
    }
    close(_clientFd);
}