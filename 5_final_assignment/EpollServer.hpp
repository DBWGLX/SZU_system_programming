#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "ThreadPool.h"

class Task1 : public Task {
public:
    void execute() override {
        // 执行任务1的具体操作
    }
};

class EpollServer{
public:
    EpollServer():threadPool(std::make_unique<ThreadPool>(4)){
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
                    
                }
            }
        }
    }
private:

    int serverFd;//套接字
    int epollFd;
    std::unique_ptr<ThreadPool> threadPool;
};