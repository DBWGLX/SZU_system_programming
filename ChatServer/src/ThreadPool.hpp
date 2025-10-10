/*
    线程池：生产者消费者模型
*/
#pragma once
//#include <pthread.h>  // For pthread functions
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <atomic>
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"

#define POOLSIZE 4 //可能调用者传参了！

// 数据库配置宏
#define DB_HOST "tcp://127.0.0.1:3306"
#define DB_USER "root"
#define DB_PASSWORD "123456"
#define DB_NAME "chatServer"
#define DB_POOL_SIZE POOLSIZE

//任务类：
class Task {
public:
    //virtual void execute();
    virtual void execute(DBOperation& dbop) = 0;
    virtual ~Task() = default;
};

//继承示例demo:
// class Task1 : public Task {
// public:
//     void execute() override {
//         // 执行任务1的具体操作
//     }
// };

//聊天服务Task在 ClientTask.hpp 文件中定义

class ThreadPool{
public:
    ThreadPool(std::atomic<bool>& interrupted, size_t numThreads = POOLSIZE);
    ~ThreadPool();
    void enqueue(Task* task);

private:
    void worker(); //线程执行逻辑
    std::vector<std::thread> workers;//线程组
    std::queue<Task*> tasks;//任务队列
    std::mutex queueMutex;//任务队列锁
    std::condition_variable condition;//条件变量阻塞

    MySQLConnectionPool mysqlPool;//【高耦合了】
    std::atomic<bool>& _interrupted;//服务器停止
};