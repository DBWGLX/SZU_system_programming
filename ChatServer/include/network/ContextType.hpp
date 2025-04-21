#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <string>


namespace dbwg{
    enum class ContextType {
        ACCEPT,
        READ,
        WRITE
    };

    class BaseContext {
    public:
        explicit BaseContext(ContextType t);
        virtual ~BaseContext() = default;
        ContextType getType() const;

    private:
        ContextType type;
    };

    class AcceptContext : public BaseContext {
    public:
        AcceptContext(int fd);
        int getListenFd() const;
        sockaddr_in& getClientAddr();
        socklen_t& getClientAddrLen();

    private:
        int listenFd;
        sockaddr_in clientAddr;
        socklen_t clientAddrLen;
    };

    class ReadContext : public BaseContext{
    public:
        // % 静态常量整数类型可以在类内直接初始化
        static const size_t HEADER_SIZE = 8; // 4字节 K + 4字节 L
        static const size_t BUFFER_SIZE = 2048;  // 默认读取大小
        
        ReadContext(int fd);
        ReadContext(const ReadContext&) = delete;
        ReadContext& operator=(const ReadContext&) = delete;
        ~ReadContext();
        void appendData(const char* data, size_t len);
        bool hasCompleteKLV() const;
        std::string extractOneKLV();
        int getClientFd() const;

        // 对外暴露 buffer
        //char* buffer;
        size_t bufferLen;//数据先接收到这里，然后追加给解析缓冲区解析
        char* buffer;
    private:
        int clientFd;
        
        std::vector<char> recvBuffer;
        size_t parsedPos = 0;
    };

    class SendContext : public BaseContext{
    public:
        SendContext(int clientFd, std::string msg);
        int sockfd;
        std::string data;  // 整个KLV数据
        size_t offset = 0; // 已发送位置
    private:
    };

    bool isAccept(void* user_data);
    bool isRead(void* user_data);
    bool isWrite(void* user_data);
}

#endif