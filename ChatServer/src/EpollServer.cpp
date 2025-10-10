#include "EpollServer.hpp"

// EpollServer 类定义

//0.初始化：网络套接字，多路复用
EpollServer::EpollServer(std::atomic<bool>& interrupted)
    : threadPool(std::make_unique<ThreadPool>(interrupted, 16)), _interrupted(interrupted)
{
    // 1.初始化 io_uring，队列深度为 256
    int ret = io_uring_queue_init(256, &ring, 0); 
    if (ret < 0) {
        fatal_str("io_uring 初始化失败: " + std::string(strerror(-ret)));
    }

    // 2.初始化套接字
    initSocket();
}

//-1.关闭文件描述符
EpollServer::~EpollServer() {
    close(epollFd);
    close(serverFd);
}

//2.运行逻辑
void EpollServer::work() {
    // 0.io_uring 监听
    auto* read_ctx = new AcceptContext(serverFd);
    submitAccept(read_ctx);

    // 1.开始工作
    while (!_interrupted) {
        debug("#### 开始一次监听");
        logger_flush();

        int ret = io_uring_submit_and_wait(&ring, 1);
        //debug_str("io_uring_submit_and_wait 返回");logger_flush();
        // 1.0.结束
        if (ret < 0) {
            if (ret == -EINTR) {
                warn_str("io_uring 发生信号中断");
                continue;
            }
            error_str("❌ io_uring_submit_and_wait 失败" + std::string(strerror(-ret)));
            break;
        }

        unsigned head;
        io_uring_cqe* cqe;
        int handled = 0; //记录本次处理数目用于提交清理

        // 1.1 处理任务
        io_uring_for_each_cqe(&ring, head, cqe) { // entry

            //1. 获取请求的数据内容
            void* user_data = io_uring_cqe_get_data(cqe);
            if (!user_data) {
                error_str("⚠️ cqe 的 user_data 是空的！");
                handled++;
                continue;
            }

            //2. 根据内容类型做处理：

            // ===== a.接收连接请求 =====
            if (isAccept(user_data)) {

                // a.1 获取新连接的描述符
                AcceptContext* ctx = static_cast<AcceptContext*>(user_data);
                int clientFd = cqe->res;
                if (clientFd <= 0) {
                    error_str("❌ accept 失败: " + std::to_string(clientFd) + " " + std::string(strerror(errno)));
                    logger_flush();
                    handled++;
                    continue;
                }

                // a.2 设置非阻塞
                int flags = fcntl(clientFd, F_GETFL, 0);
                if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    fatal_str("❌ 设置非阻塞失败");
                    close(clientFd);
                    delete ctx;
                    handled++;
                    continue;
                }

                //（打印连接信息
                {
                    char clientIp[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(ctx->getClientAddr().sin_addr), clientIp, INET_ADDRSTRLEN);
                    int clientPort = ntohs(ctx->getClientAddr().sin_port);
                    std::ostringstream oss;
                    oss << "🕊️ 收到客户端连接请求: IP: " << clientIp
                        << ", 端口: " << clientPort
                        << ", clientFd: " << clientFd;
                    info_str(oss.str());
                }

                // a.3 加入监听
                auto* read_ctx = new ReadContext(clientFd);
                submitRead(clientFd, read_ctx);     // 启动该客户端的读取

                submitAccept(ctx); // 继续监听下一个连接
            } 
            // ===== b.解析客户端发送的数据 =====
            else if (isRead(user_data)) {
                debug("正在处理接收到的数据");

                ReadContext* ctx = static_cast<ReadContext*>(user_data);

                //b.1 关闭连接/读取失败
                if (cqe->res <= 0) {
                    if (cqe->res == 0) {
                        info_str("🔌 客户端主动关闭连接 " + std::string(strerror(-cqe->res)));
                    } else {
                        error_str("❌ 读取失败: " + std::string(strerror(-cqe->res)));
                    }
                    close(ctx->getClientFd());
                    delete ctx;// 删除读缓冲区
                    handled++;
                    continue;
                }

                //b.2 数据先迁移到 解析缓冲区
                ctx->appendData(ctx->buffer, cqe->res);

                //b.3 解析
                while (ctx->hasCompleteKLV()) {
                    auto msg = ctx->extractOneKLV();
                    //将消息放入任务队列
                    threadPool->enqueue(new ClientTask(ctx->getClientFd(), msg, serverFd, &_LRUm, &ring));
                }

                //b.4 继续提交下一次 read
                submitRead(ctx->getClientFd(), ctx);
            }
            // ===== c.转发客户端发送的消息 =====
            else if (isWrite(user_data)){
                SendContext* ctx = static_cast<SendContext*>(io_uring_cqe_get_data(cqe));
                ssize_t sent = cqe->res;

                if (sent < 0) {
                    std::cerr << "发送失败，错误码: " << -sent << std::endl;
                    delete ctx;
                    return;
                }

                ctx->offset += sent;
                submitSend(ctx);  // 继续发送剩余数据 or 结束释放
            }

            handled++;
        }

        // 1.2 批量标记所有处理完的 cqe
        io_uring_cq_advance(&ring, handled);
    }
    
    // -1.结束
    delete read_ctx;
    if (errno == EINTR) {
        std::cerr << "epoll_wait interrupted by signal SIGINT." << std::endl;
    }
}

//1.网络套接字初始化
void EpollServer::initSocket() {

    ///1.初始化服务器套接字描述符
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    //Address Family: 还有 AF_INET6 ，本地AF_UNIX / AF_LOCAL
    //SOCK_STREAM: TCP流式传输；数据包: SOCK_DGRAM
    //protocol=0: 自动匹配 TCP 协议
    if (serverFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }
    // 1.2 设置非阻塞
    int flags = fcntl(serverFd, F_GETFL, 0);//获取当前标志
    if(fcntl(serverFd, F_SETFL, flags | O_NONBLOCK)==-1){ 
        std::ostringstream oss; //【流】优点：自动完成int转换；性能差一点
        oss << "❌ 设置非阻塞失败 serverFd:" << serverFd;
        fatal_str(oss.str());
    }
    // 1.3 设置接收缓冲区大小
    int recvBufSize = 16 * 1024 * 1024; // 16MB
    if(setsockopt(serverFd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize)) == -1){
        perror("setsockopt");
    }

    // 1.4 设置套接字配置
    struct sockaddr_in serverAddr{};
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; //服务器会绑定到所有可用的网络接口（即本机的所有 IP 地址）
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverFd,  (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(serverFd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(serverFd, SOMAXCONN) == -1) { // 设置为监听状态
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    debug("服务器套接字开始监听，serverFd: " + std::to_string(serverFd));
}

/// io_uring封装：
// 放入提交队列 + 提交
void EpollServer::submitAccept(AcceptContext* ctx) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fatal_str("❌ 获取 SQE 失败，可能 ring 满了");
        return;
    }

    io_uring_prep_accept(sqe,serverFd,
        reinterpret_cast<sockaddr*>(&ctx->getClientAddr()),&ctx->getClientAddrLen(),0);

    io_uring_sqe_set_data(sqe, ctx); // 设置上下文
    // 在 SQ【Submission Queue提交队列】放入一个任务 （最终需批量提交

    // 批量提交：真正刷新到内核
    if (io_uring_submit(&ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        //delete ctx;
        return;
    }
}


// 提交读写请求
// 准备一个 SQE（submission queue entry）
void EpollServer::submitRead(int clientFd, ReadContext* ctx) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fatal_str("❌ 获取 read SQE 失败");
        //delete ctx;
        return;
    }

    io_uring_prep_read(sqe, clientFd, ctx->buffer, ctx->bufferLen, 0);
    io_uring_sqe_set_data(sqe, ctx);
    //只是准备好了 SQE（submission queue entry）
    
    if (io_uring_submit(&ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        //delete ctx;
        return;
    }
}


void EpollServer::submitSend(SendContext* ctx) {
    size_t remaining = ctx->data.size() - ctx->offset;
    if (remaining == 0) {
        delete ctx; // 发送完毕，释放资源
        return;
    }

    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fatal_str("❌ 获取 send SQE 失败");
        return;
    }

    // 发送时的指针
    void* ptr = (void*)(ctx->data.data() + ctx->offset);
    size_t len = remaining;

    io_uring_prep_send(sqe, ctx->sockfd, ptr, len, 0);
    io_uring_sqe_set_data(sqe, ctx); // 传回这个 ctx

    if (io_uring_submit(&ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        //delete ctx;
        return;
    }
}