#include "EpollServer.hpp"

// EpollServer 类定义

EpollServer::EpollServer()
    : threadPool(std::make_unique<ThreadPool>(4)),
      mysqlPool("tcp://127.0.0.1:3306", "root", "123456", "chatServer", 5),
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

        for (int i = 0; i < readyFdCount; i++) {
            if (events[i].data.fd == serverFd) {
                sockaddr_in clientAddr{};
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), reinterpret_cast<socklen_t*>(&clientAddrLen));
                if (clientFd == -1) {
                    perror("accept");
                    continue;
                }

                epoll_event clientEvent{};
                clientEvent.data.fd = clientFd;
                clientEvent.events = EPOLLIN;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
                    perror("epoll_ctl");
                    close(clientFd);
                }
            } else {
                threadPool->enqueue(new ClientTask(events[i].data.fd, epollFd, &dbop, &LRUm));
            }
        }
    }
}

void EpollServer::initSocket() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    if (bind(serverFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
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
}
