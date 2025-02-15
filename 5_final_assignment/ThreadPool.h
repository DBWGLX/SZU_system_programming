#pragma once
#include <bits/stdc++.h>
using namespace std;
#define POOLSIZE 4

class Task {
public:
    virtual void execute() = 0;
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
    static void* worker(void*arg);
    std::vector<pthread_t> workers;
    std::queue<Task*> tasks;
    pthread_mutex_t queueMutex;
    pthread_cond_t condition;
    bool stop;
};

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    pthread_mutex_init(&queueMutex, nullptr);
    pthread_cond_init(&condition, nullptr);

    for (size_t i = 0; i < numThreads; ++i) {
        pthread_t thread;
        pthread_create(&thread, nullptr, worker, this);
        workers.push_back(thread);
    }
}

ThreadPool::~ThreadPool() {
    {
        pthread_mutex_lock(&queueMutex);
        stop = true;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&queueMutex);
    }
    for (pthread_t& worker : workers) {
        pthread_join(worker, nullptr);
    }
    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&condition);
}

void ThreadPool::enqueue(Task* task) {
    pthread_mutex_lock(&queueMutex);
    tasks.push(task);
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&queueMutex);
}

void* ThreadPool::worker(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (true) {
        Task* task;
        {
            pthread_mutex_lock(&pool->queueMutex);
            while (!pool->stop && pool->tasks.empty()) {
                pthread_cond_wait(&pool->condition, &pool->queueMutex);
            }
            if (pool->stop && pool->tasks.empty()) {
                pthread_mutex_unlock(&pool->queueMutex);
                break;
            }
            task = pool->tasks.front();
            pool->tasks.pop();
            pthread_mutex_unlock(&pool->queueMutex);
        }
        task->execute();  // 执行任务
        delete task;  // 任务完成后删除
    }
    return nullptr;
}