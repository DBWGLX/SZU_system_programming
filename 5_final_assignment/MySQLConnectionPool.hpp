#ifndef MySQL
#define MySQL
#include <iostream>
#include <queue>

#include <mutex>
#include <thread>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#endif


class MySQLConnectionPool{
public:
    MySQLConnectionPool(const std::string& host,const std::string& user,const std::string& password, 
                        const std::string& database,size_t pool_size = 10)
        :_host(host),_user(user),_password(password),_database(database),_pool_size(pool_size){
        for(size_t i = 0;i < _pool_size; ++i){
            std::shared_ptr<sql::Connection> conn(createConnection());
            _connection_pool.push(conn);
        }
    }
    std::shared_ptr<sql::Connection> getConnection(){
        std::lock_guard<std::mutex> lock(_pool_mutex_o);
        if(!_connection_pool.empty()){
            std::shared_ptr<sql::Connection> conn = _connection_pool.front();
            _connection_pool.pop();
            return conn;
        }else{
            _pool_size++;
            std::cerr<<"【】Connection pool is empty, creating a new connection! Now _pool_size: " 
                <<  _pool_size <<std::endl;
            return createConnection();
        }
    }
    void releaseConnection(std::shared_ptr<sql::Connection> conn){
        std::lock_guard<std::mutex>lock(_pool_mutex_i);
        if(conn){
            if(_pool_size > 20){
                _pool_size--;
            }else{
                _connection_pool.push(conn);
            }
        }
    }
private:
    std::shared_ptr<sql::Connection> createConnection(){
        sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
        return std::shared_ptr<sql::Connection>(driver->connect(_host,_user,_password));
    }

    std::string _host;
    std::string _user;
    std::string _password;
    std::string _database;
    size_t _pool_size;
    std::queue<std::shared_ptr<sql::Connection>> _connection_pool;
    std::mutex _pool_mutex_i,_pool_mutex_o;
};