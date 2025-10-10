#include "ThreadPool.hpp"

ThreadPool::ThreadPool(std::atomic<bool>& interrupted, size_t numThreads) 
    : mysqlPool(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_POOL_SIZE), _interrupted(interrupted){

    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::worker, this);
    }

    std::cout << "线程池创建完成，线程数量: " << workers.size() << std::endl;
    for (size_t i = 0; i < workers.size(); ++i) {
        std::cout << "线程 " << i << " ID: " << workers[i].get_id() << std::endl;
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

    DBOperation dbop(mysqlPool.getConnection());//获取线程专属数据库连接；其实应调用对应类的方法

    while (!_interrupted) {
        Task* task = nullptr;
        //1.等待任务
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return _interrupted || !tasks.empty(); });
            if (_interrupted && tasks.empty()) {
                break;
            }
            task = tasks.front();
            tasks.pop();
        }
        //2.执行任务
        if (task) {
            task->execute(dbop);
            delete task;
        }
    }
}