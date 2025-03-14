#include "MySQLConnectionPool.hpp"

// 构造函数
MySQLConnectionPool::MySQLConnectionPool(const std::string& host, const std::string& user, 
                                         const std::string& password, const std::string& database, 
                                         size_t pool_size)
    : _host(host), _user(user), _password(password), _database(database), _pool_size(pool_size) {
    for (size_t i = 0; i < _pool_size; ++i) {
        std::shared_ptr<sql::Connection> conn(createConnection());
        _connection_pool.push(conn);
    }
}

// 获取数据库连接
std::shared_ptr<sql::Connection> MySQLConnectionPool::getConnection() {
    std::lock_guard<std::mutex> lock(_pool_mutex_o);

    if (!_connection_pool.empty()) {
        std::shared_ptr<sql::Connection> conn = _connection_pool.front();
        _connection_pool.pop();
        return conn;
    } else {
        _pool_size++;
        //std::cerr << "Connection pool is empty, creating a new connection! Now _pool_size: " <<  _pool_size << std::endl;
        return createConnection();
    }
}

// 释放数据库连接
void MySQLConnectionPool::releaseConnection(std::shared_ptr<sql::Connection> conn) {
    std::lock_guard<std::mutex> lock(_pool_mutex_i);
    if (conn) {
        if (_pool_size > 20) {
            _pool_size--;
        } else {
            _connection_pool.push(conn);
        }
    }
}

// 创建一个新的数据库连接
std::shared_ptr<sql::Connection> MySQLConnectionPool::createConnection() {
    sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
    return std::shared_ptr<sql::Connection>(driver->connect(_host, _user, _password));
}
