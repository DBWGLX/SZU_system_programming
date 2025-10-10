/*
连接，通信消息 的封装

*/
#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <string>


namespace dbwg{
    //枚举类型
    enum class ContextType {
        ACCEPT,
        READ,
        WRITE
    };

    //根类型
    class BaseContext {
    public:
        explicit BaseContext(ContextType t);
        virtual ~BaseContext() = default;
        ContextType getType() const;

    private:
        ContextType type;//消息类型
    };

    //1.连接请求
    class AcceptContext : public BaseContext {
    public:
        AcceptContext(int fd);
        int getListenFd() const;
        sockaddr_in& getClientAddr();
        socklen_t& getClientAddrLen();

    private:
        int listenFd;//文件描述符
        //套接字
        sockaddr_in clientAddr;
        socklen_t clientAddrLen;
    };

    //2.客户端消息
    /*
        虽然是 klv，可以先读长度再读内容，但是这需要【两次系统调用】
        应先读取，再解析。所以需要维护缓冲区处理逻辑
    */
    class ReadContext : public BaseContext{
    public:
        //0.消息配置
        // % 静态常量整数类型可以在类内直接初始化
        static const size_t HEADER_SIZE = 8; // 4字节 K + 4字节 L
        static const size_t BUFFER_SIZE = 2048;  // 默认读取大小
        
        //1.
        ReadContext(int fd);
        ReadContext(const ReadContext&) = delete;
        ReadContext& operator=(const ReadContext&) = delete;
        ~ReadContext();
        //1.1操作
        void appendData(const char* data, size_t len);
        bool hasCompleteKLV() const;//检查
        std::string extractOneKLV();//解析提取
        int getClientFd() const;

        //2.对外暴露 buffer
        //char* buffer;
        size_t bufferLen;//数据先接收到这里，然后追加给解析缓冲区解析
        char* buffer;
    private:
        int clientFd;
        
        std::vector<char> recvBuffer;
        size_t parsedPos = 0; //数据解析游标
    };

    //3.服务器推送消息
    class SendContext : public BaseContext{
    public:
        SendContext(int clientFd, std::string msg);
        int sockfd;
        std::string data;  // 整个KLV数据
        size_t offset = 0; // 已发送位置
    private:
    };


    //4.类型检查
    bool isAccept(void* user_data);
    bool isRead(void* user_data);
    bool isWrite(void* user_data);
}

#endif