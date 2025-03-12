#pragma once

#include <sstream>
#include "MySQLConnectionPool.hpp"
#include "User.hpp"
#include "Logger.hpp"

class DBOperation{
public:
    DBOperation(MySQLConnectionPool* pool)
        :_pool(pool){
    }
    int addUser(const User& new_user){
        try {
            auto conn = pool->getConnection();
            std::string sqlSentence = "INSERT INTO users (account, password, username, phone_number, email) VALUES (?, ?, ?, ?, ?)";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sqlSentence));
            //设置参数
            stmt->setString(1,new_user.getAccount());
            stmt->setString(2, new_user.getPassword());
            stmt->setString(3, new_user.getUsername());
            stmt->setString(4, new_user.getPhoneNumber());
            stmt->setString(5, new_user.getEmail());
            //执行
            stmt->executeUpdate();
            pool->releaseConnection(conn);
            return 0;
        } catch (sql::SQLException& e) {
            //检查错误码
            if(e.getErrorCode() == 1062){ //MySQL 唯一键冲突错误码
                return -1;
            }
            std::cerr << "Error: " << e.what() << std::endl;
            fatal_str(e.what());
            return -2;//其他错误
        }
    }
    bool verifyUser(const std::string& account, const std::string& inputPassword) {
        try {
            std::string sql = "SELECT password FROM users WHERE account = ?";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            stmt->setString(1, account);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

            if (res->next()) {
                std::string storedPassword = res->getString("password");
                return storedPassword == inputPassword;
            }
        } catch (sql::SQLException& e) {
            std::cerr << "Query error: " << e.what() << std::endl;
            fatal_str(e.what());
        }
        return false;
    }
    int updateUserInfo(const User& user) {
        try {
            auto conn = _pool->getConnection();
            std::string sql = "UPDATE users SET password = ?, username = ?, phone_number = ?, email = ? WHERE account = ?";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            stmt->setString(1, user.getPassword());
            stmt->setString(2, user.getUsername());
            stmt->setString(3, user.getPhoneNumber());
            stmt->setString(4, user.getEmail());
            stmt->setString(5, user.getAccount());
            stmt->executeUpdate();
            _pool->releaseConnection(conn);
            return 0;
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            fatal_str(e.what());
            return -1;
        }
    }
    int deleteUser(const std::string& account) {
        try {
            auto conn = _pool->getConnection();
            std::string sql = "DELETE FROM users WHERE account = ?";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            stmt->setString(1, account);
            int rowsAffected = stmt->executeUpdate();
            _pool->releaseConnection(conn);
            if (rowsAffected > 0) {
                return 0; // 删除成功
            } else {
                return -2; // 用户不存在
            }
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1; // 其他错误
        }
    }
private:
    MySQLConnectionPool* _pool;
};