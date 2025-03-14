#pragma once
#include <pthread.h>  // For pthread functions
#include <vector>     // For std::vector
#include <queue>      // For std::queue

#define POOLSIZE 4

class Task {
public:
    virtual void execute() = 0;
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
    static void* worker(void*arg); //线程方法
    std::vector<pthread_t> workers;
    pthread_mutex_t queueMutex;
    pthread_cond_t condition;
    std::queue<Task*> tasks;//方法任务队列
    bool stop;
};