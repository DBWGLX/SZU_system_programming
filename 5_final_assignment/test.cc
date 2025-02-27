#include<iostream>
#include<unistd.h>
//测试数据库
#include "MySQLConnectionPool.hpp"
#include <cppconn/exception.h>
void threadFunction(MySQLConnectionPool& pool) {
    try {
        auto conn = pool.getConnection();
        std::cout << "Connection acquired in thread: " << std::this_thread::get_id() << std::endl;

        // Example of using the connection
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement("SELECT 1"));
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        while (res->next()) {
            std::cout << "Query result: " << res->getInt(1) << std::endl;
        }

        pool.releaseConnection(conn);
        std::cout << "Connection released in thread: " << std::this_thread::get_id() << std::endl;
    } catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    MySQLConnectionPool pool("tcp://127.0.0.1:3306", "root", "123456", "chatServer", 5);

    // Spawn multiple threads to simulate concurrent database access
    std::vector<std::thread> threads;
    for (int i = 0; i < 300; ++i) {
        threads.emplace_back(threadFunction, std::ref(pool));//ref是应对线程拷贝函数参数。
    }

    for (auto& t : threads) {
        t.join();
    }
    return 0;
}