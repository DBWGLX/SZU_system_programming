#pragma once
//#include <pthread.h>  // For pthread functions
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>     // For std::vector
#include <queue>      // For std::queue
#include "MySQLConnectionPool.hpp"
#include "DBOperation.hpp"

#define POOLSIZE 4

// 数据库配置宏
#define DB_HOST "tcp://127.0.0.1:3306"
#define DB_USER "root"
#define DB_PASSWORD "123456"
#define DB_NAME "chatServer"
#define DB_POOL_SIZE POOLSIZE


class Task {
public:
    //virtual void execute();
    virtual void execute(DBOperation& dbop) = 0;
    virtual ~Task() = default;
};

// class Task1 : public Task {
// public:
//     void execute() override {
//         // 执行任务1的具体操作
//     }
// };

class ThreadPool{
public:
    ThreadPool(size_t numThreads = POOLSIZE);
    ~ThreadPool();
    void enqueue(Task* task);

private:
    void worker(); //线程方法
    std::vector<std::thread> workers;
    std::mutex queueMutex;
    std::condition_variable condition;
    //std::vector<pthread_t> workers;
    //pthread_mutex_t queueMutex;
    //pthread_cond_t condition;
    std::queue<Task*> tasks;//方法任务队列

    MySQLConnectionPool mysqlPool;
    bool stop;
};