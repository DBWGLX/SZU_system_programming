#include "EpollServer.hpp"

// EpollServer 类定义

EpollServer::EpollServer(std::atomic<bool>& interrupted)
    : threadPool(std::make_unique<ThreadPool>(interrupted, 4)), _interrupted(interrupted)
{
    int ret = io_uring_queue_init(256, &ring, 0); // 初始化 io_uring，队列深度为 256
    if (ret < 0) {
        fatal_str("io_uring 初始化失败: " + std::string(strerror(-ret)));
    }

    initSocket();
}

EpollServer::~EpollServer() {
    close(epollFd);
    close(serverFd);
}

void EpollServer::work() {
    auto* read_ctx = new AcceptContext(serverFd);
    // io_uring 监听
    submitAccept(read_ctx);

    while (!_interrupted) {
        debug("#### 开始一次监听");
        logger_flush();

        int ret = io_uring_submit_and_wait(&ring, 1);
        //debug_str("io_uring_submit_and_wait 返回");logger_flush();
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
        int handled = 0;

        io_uring_for_each_cqe(&ring, head, cqe) { // entry

            //获取 data
            void* user_data = io_uring_cqe_get_data(cqe);
            if (!user_data) {
                error_str("⚠️ cqe 的 user_data 是空的！");
                handled++;
                continue;
            }

            // ===== 接收连接请求 =====
            if (isAccept(user_data)) {

                auto* ctx = static_cast<AcceptContext*>(user_data);
                int clientFd = cqe->res;

                if (clientFd <= 0) {
                    error_str("❌ accept 失败: " + std::to_string(clientFd) + " " + std::string(strerror(errno)));
                    logger_flush();
                    handled++;
                    continue;
                }

                // 设置非阻塞
                int flags = fcntl(clientFd, F_GETFL, 0);
                if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    fatal_str("❌ 设置非阻塞失败");
                    close(clientFd);
                    delete ctx;
                    handled++;
                    continue;
                }


                // 打印连接信息
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


                auto* read_ctx = new ReadContext(clientFd);
                submitRead(clientFd, read_ctx);     // 启动该客户端的读取

                submitAccept(ctx); // 继续监听下一个连接
            } 
            // ===== 客户端发送数据 =====
            else if (isRead(user_data)) {
                debug("正在处理接收到的数据");

                auto* ctx = static_cast<ReadContext*>(user_data);

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

                // 数据先迁移到 解析缓冲区
                ctx->appendData(ctx->buffer, cqe->res);

                // 解析
                while (ctx->hasCompleteKLV()) {
                    auto msg = ctx->extractOneKLV();
                    threadPool->enqueue(new ClientTask(ctx->getClientFd(), msg, serverFd, &_LRUm));
                }

                // 继续提交下一次 read
                submitRead(ctx->getClientFd(), ctx);
            }


            handled++;
        }
        // 批量标记所有处理完的 cqe
        io_uring_cq_advance(&ring, handled);
    }
    
    delete read_ctx;
    if (errno == EINTR) {
        std::cerr << "epoll_wait interrupted by signal SIGINT." << std::endl;
    }
}


void EpollServer::initSocket() {
    ///初始化服务器套接字
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("socket");
        throw std::runtime_error("Failed to create socket");
    }
    int flags = fcntl(serverFd, F_GETFL, 0);//获取当前标志
    if(fcntl(serverFd, F_SETFL, flags | O_NONBLOCK)==-1){
        std::ostringstream oss;
        oss << "❌ 设置非阻塞失败 serverFd:" << serverFd;
        fatal_str(oss.str());
    }
    //设置接收缓冲区
    int recvBufSize = 16 * 1024 * 1024; // 16MB
    if(setsockopt(serverFd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize)) == -1){
        perror("setsockopt");
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
    if (listen(serverFd, SOMAXCONN) == -1) { // 设置为监听状态
        close(serverFd);
        throw std::runtime_error("Failed to listen on socket");
    }

    debug("服务器套接字开始监听，serverFd: " + std::to_string(serverFd));
}

// Submission Queue
void EpollServer::submitAccept(AcceptContext* ctx) {

    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fatal_str("❌ 获取 SQE 失败，可能 ring 满了");
        return;
    }

    io_uring_prep_accept(sqe,serverFd,
        reinterpret_cast<sockaddr*>(&ctx->getClientAddr()),&ctx->getClientAddrLen(),0);

    io_uring_sqe_set_data(sqe, ctx); // 设置上下文
    if (io_uring_submit(&ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        //delete ctx;
        return;
    }
}


// 提交读写请求
void EpollServer::submitRead(int clientFd, ReadContext* ctx) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        fatal_str("❌ 获取 read SQE 失败");
        //delete ctx;
        return;
    }

    io_uring_prep_read(sqe, clientFd, ctx->buffer, ctx->bufferLen, 0);
    io_uring_sqe_set_data(sqe, ctx);
    
    if (io_uring_submit(&ring) < 0) {
        fatal_str("❌ io_uring_submit 提交失败");
        //delete ctx;
        return;
    }
}


