#include "ThreadPool.hpp"

ThreadPool::ThreadPool(std::atomic<bool>& interrupted, size_t numThreads) 
    : mysqlPool(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_POOL_SIZE), _interrupted(interrupted){

    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        condition.notify_all();
    }
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::enqueue(Task* task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(task);
    }
    condition.notify_one();
}

void ThreadPool::worker() { // void worker(ThreadPool* this, Connection conn);
    DBOperation dbop(mysqlPool.getConnection());
    while (!_interrupted) {
        Task* task = nullptr;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return _interrupted || !tasks.empty(); });
            if (_interrupted && tasks.empty()) {
                break;
            }
            task = tasks.front();
            tasks.pop();
        }
        if (task) {
            task->execute(dbop);
            delete task;
        }
    }
}