#ifndef MYSQL_CONNECTION_POOL_HPP
#define MYSQL_CONNECTION_POOL_HPP

#include <iostream>
#include <queue>
#include <memory>
#include <mutex>
#include <thread>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

class MySQLConnectionPool {
public:
    MySQLConnectionPool(const std::string& host, const std::string& user, const std::string& password, 
                        const std::string& database, size_t pool_size = 10);

    std::shared_ptr<sql::Connection> getConnection();
    void releaseConnection(std::shared_ptr<sql::Connection> conn);

private:
    std::shared_ptr<sql::Connection> createConnection();

    std::string _host;
    std::string _user;
    std::string _password;
    std::string _database;
    size_t _pool_size;
    std::queue<std::shared_ptr<sql::Connection>> _connection_pool;
    std::mutex _pool_mutex_i, _pool_mutex_o;
};

#endif // MYSQL_CONNECTION_POOL_HPP
