#include "ContextType.hpp"
#include <cstring>

namespace dbwg{
    BaseContext::BaseContext(ContextType t) : type(t) {}

    ContextType BaseContext::getType() const {
        return type;
    }

    ///1.连接请求
    AcceptContext::AcceptContext(int fd)
        : BaseContext(ContextType::ACCEPT), listenFd(fd), clientAddrLen(sizeof(clientAddr)) {}

    int AcceptContext::getListenFd() const {
        return listenFd;
    }
    sockaddr_in& AcceptContext::getClientAddr(){
        return clientAddr;
    }
    socklen_t& AcceptContext::getClientAddrLen(){
        return clientAddrLen;
    }

    ///2.客户端消息
    ReadContext::ReadContext(int fd) 
        : BaseContext(ContextType::READ), bufferLen(BUFFER_SIZE), clientFd(fd){
        buffer = new char[bufferLen];
        recvBuffer.reserve(BUFFER_SIZE*2);//预留2倍空间
    }

    ReadContext::~ReadContext() {
        delete[] buffer;
    }
    void ReadContext::appendData(const char* data, size_t len) {
        recvBuffer.insert(recvBuffer.end(), data, data + len);
    }
    bool ReadContext::hasCompleteKLV() const {
        size_t remain = recvBuffer.size() - parsedPos;
        if (remain < HEADER_SIZE) return false;

        uint32_t len;
        std::memcpy(&len, &recvBuffer[parsedPos + 4], 4);
        len = ntohl(len);

        if (remain < HEADER_SIZE + len) return false;

        return true;
    }
    std::string ReadContext::extractOneKLV() {
        //1.获取类型key
        //uint32_t key = (int*)recvBuffer[parsedPos];
        
        //2.获取长度len
        uint32_t len;
        std::memcpy(&len, &recvBuffer[parsedPos + 4], 4);
        len = ntohl(len);

        //3.读取对应长度数据
        std::string ret = std::string(&recvBuffer[parsedPos], HEADER_SIZE + len);
        parsedPos += HEADER_SIZE + len;

        //4.定期清理*
        if (parsedPos == recvBuffer.size()) {//报文大小固定？
            recvBuffer.clear();
            parsedPos = 0;
        } else if (parsedPos > BUFFER_SIZE) {//过半时，将新消息保留后清理
            std::vector<char> tmp(recvBuffer.begin() + parsedPos, recvBuffer.end());
            recvBuffer = std::move(tmp);//move: 旧内存会被自动释放；被move的对象内部值未定义
            parsedPos = 0;
        }

        return ret;
    }
    int ReadContext::getClientFd() const {
        return clientFd;
    }

    ///3.服务器推送消息
    SendContext::SendContext(int clientFd, std::string msg)
        : BaseContext(ContextType::WRITE), sockfd(clientFd), data(msg){
    }

    ///
    bool isAccept(void* user_data) {
        auto* base = static_cast<BaseContext*>(user_data);
        return base->getType() == ContextType::ACCEPT;
    }
    bool isRead(void* user_data) {
        auto* base = static_cast<BaseContext*>(user_data);
        return base->getType() == ContextType::READ;
    }
    bool isWrite(void* user_data) {
        auto* base = static_cast<BaseContext*>(user_data);
        return base->getType() == ContextType::WRITE;
    }
}