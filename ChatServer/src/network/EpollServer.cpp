#include "EpollServer.hpp"

// EpollServer 类定义

EpollServer::EpollServer(std::atomic<bool>& interrupted)
    : threadPool(std::make_unique<ThreadPool>(interrupted, 4)), _interrupted(interrupted){
    initSocket();
}

EpollServer::~EpollServer() {
    close(epollFd);
    close(serverFd);
}

void EpollServer::work() {
    while (!_interrupted) {
        logger_flush();

        struct epoll_event events[10];
        int readyFdCount = epoll_wait(epollFd, events, 10, -1);
        if (readyFdCount == -1) {
            if (errno == EINTR) {
                continue; // 被信号中断，直接继续循环检查 interrupted
            }
            throw std::runtime_error("epoll_wait error");
            break;
        }

        //info_str("📨 服务器收到请求");
        
        //处理IO时间
        handleEpollEvents(events, readyFdCount);
    }
    
    if (errno == EINTR) {
        std::cerr << "epoll_wait interrupted by signal SIGINT." << std::endl;
    }
}


//处理IO
void EpollServer::handleEpollEvents(struct epoll_event* events, int readyFdCount){
    for (int i = 0; i < readyFdCount; i++) {
        if(events[i].events & (EPOLLERR | EPOLLHUP)){ // 连接半关闭状态；对方已关闭
            std::ostringstream oss;
            oss << "Client connection closed or error occurred" << std::endl;
            fatal_str(oss.str());
            int clientFd = events[i].data.fd;
            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1) {
                std::cerr << "Failed to remove clientFd from epoll instance: " << strerror(errno) << std::endl;
            }
            close(clientFd);
        }//EPOLLHUP
        else if (events[i].data.fd == serverFd) { //服务器接收到连接请求
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), reinterpret_cast<socklen_t*>(&clientAddrLen));
            if (clientFd == -1) {
                perror("accept");
                continue;
            }

            // 打印客户端的 IP 地址和端口信息
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            std::ostringstream oss;
            oss << "📨 收到客户端连接请求: IP: " << clientIp << ", 端口: " << clientPort << ", clientFd:" << clientFd;
            info_str(oss.str());

            epoll_event clientEvent{};
            clientEvent.data.fd = clientFd;
            clientEvent.events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLONESHOT;//
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
                oss = std::ostringstream();
                oss << "❌ 连接失败：" << clientIp << ", 端口 = " << clientPort;
                fatal_str(oss.str());
                close(clientFd);
            }
        } 
        else { //客户端IO
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            // 使用 getpeername 获取客户端地址信息
            if (getpeername(events[i].data.fd, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen) == -1) {
                fatal_str("getpeername error");
                continue;
            }
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            std::ostringstream oss;
            oss << "📨 收到客户端消息: IP: " << clientIp << ", 端口: " << clientPort << ", clientFd: " << events[i].data.fd;
            info_str(oss.str());

            threadPool->enqueue(new ClientTask(events[i].data.fd, epollFd, &LRUm));
        }
    }
}


void EpollServer::initSocket() {
    ///初始化服务器套接字
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }
    struct sockaddr_in serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; //服务器会绑定到所有可用的网络接口（即本机的所有 IP 地址）
    serverAddr.sin_port = htons(SERVER_PORT);
    //设置接收缓冲区
    int recvBufSize = 16 * 1024 * 1024; // 16MB
    setsockopt(serverFd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));

    if (bind(serverFd,  (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(serverFd, SOMAXCONN) == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }


    ///初始化epoll
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll_create1");
        close(serverFd);
        throw std::runtime_error("Failed to create epoll instance");
    }

    //epoll监听服务器
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd , &ev) == -1) {
        perror("epoll_ctl");
        close(serverFd);
        close(epollFd);
        throw std::runtime_error("Failed to epoll_add serverFd");
    }
}
