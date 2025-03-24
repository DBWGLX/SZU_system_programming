#include "EpollServer.hpp"

// EpollServer 类定义

EpollServer::EpollServer()
    : threadPool(std::make_unique<ThreadPool>(4)),
      mysqlPool(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_POOL_SIZE),
      dbop(&mysqlPool) {
    initSocket();
}

EpollServer::~EpollServer() {
    close(epollFd);
    close(serverFd);
}

void EpollServer::work(std::atomic<bool>& interrupted) {
    while (!interrupted) {
        struct epoll_event events[10];
        int readyFdCount = epoll_wait(epollFd, events, 10, -1);
        if (readyFdCount == -1) {
            if (errno == EINTR) {
                std::cerr << "epoll_wait interrupted by signal SIGINT." << std::endl;
                fatal_str("🛑 Service terminated.");
                continue; // 被信号中断，直接继续循环检查 interrupted
            }
            throw std::runtime_error("epoll_wait error");
            break;
        }

        //info_str("📨 服务器收到请求");
        //
        handleEpollEvents(events, readyFdCount);

        logger_flush();
    }
}

void EpollServer::initSocket() {
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
    if (bind(serverFd,  (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(serverFd, SOMAXCONN) == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll_create1");
        close(serverFd);
        throw std::runtime_error("Failed to create epoll instance");
    }

    //居然没添加自己哈哈
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

void EpollServer::handleEpollEvents(struct epoll_event* events, int readyFdCount){
    for (int i = 0; i < readyFdCount; i++) {
        if(events[i].events & (EPOLLHUP | EPOLLERR)){ //异常情况处理
            // 连接被对方关闭或发生错误
            std::ostringstream oss;
            oss << "Client connection closed or error occurred" << std::endl;
            fatal_str(oss.str());
            int clientFd = events[i].data.fd;
            // 从 epoll 中删除并关闭文件描述符
            if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1) {
                std::cerr << "Failed to remove clientFd from epoll instance: " << strerror(errno) << std::endl;
            }
            close(clientFd);
        }
        else if (events[i].data.fd == serverFd) {//服务器接收到连接请求
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), reinterpret_cast<socklen_t*>(&clientAddrLen));
            if (clientFd == -1) {
                perror("accept");
                continue;
            }

            // 获取客户端的 IP 地址和端口信息
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            // 构建包含客户端信息的消息
            std::ostringstream oss;
            oss << "📨 收到来自客户端的连接请求: IP = " << clientIp << ", 端口 = " << clientPort << "  服务器分配的 clientFd 为：" << clientFd;
            info_str(oss.str());

            epoll_event clientEvent{};
            clientEvent.data.fd = clientFd;
            clientEvent.events = EPOLLIN| EPOLLET | EPOLLHUP | EPOLLERR;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
                oss << "❌ 连接失败：" << clientIp << ", 端口 = " << clientPort;
                fatal_str(oss.str());
                close(clientFd);
            }
        } 
        else {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);
            // 使用 getpeername 获取客户端地址信息
            if (getpeername(events[i].data.fd, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen) == -1) {
                fatal_str("getpeername error");
                continue;
            }
            // 获取客户端的 IP 地址和端口信息
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            std::ostringstream oss;
            oss << "📨 收到来自客户端的消息: IP = " << clientIp << ", 端口 = " << clientPort << "  服务器分配的 clientFd 为：" << events[i].data.fd;
            info_str(oss.str());

            threadPool->enqueue(new ClientTask(events[i].data.fd, epollFd, &dbop, &LRUm));
        }
    }
}