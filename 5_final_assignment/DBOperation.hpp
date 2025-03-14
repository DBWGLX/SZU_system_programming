#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>      // 用于 std::ostringstream
#include <stdexcept>    // 用于 std::runtime_error
#include <openssl/evp.h>
#include <openssl/rand.h>
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
            std::string sqlSentence = "INSERT INTO users (account, password, salt, username, phone_number, email) VALUES (?, ?, ?, ?, ?, ?)";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sqlSentence));

            std::string salt = generateSalt();
            std::string hashed = hashPassword(password, salt);

            //设置参数
            stmt->setString(1, new_user.getAccount());
            stmt->setString(2, toHex(hashed));
            stmt->setString(3, toHex(salt));
            stmt->setString(4, new_user.getUsername());
            stmt->setString(5, new_user.getPhoneNumber());
            stmt->setString(6, new_user.getEmail());
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
            auto conn = pool->getConnection();
            std::string sql = "SELECT password, salt FROM users WHERE account = ?";
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            stmt->setString(1, account);
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

            if (res->next()) {
                std::string storedPassword = res->getString("password");
                std::string storedSalt = res->getString("salt");
                return verifyPassword(inputPassword, fromHex(storedPassword), fromHex(storedHash));
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
    int addMessage(const std::string& send_account, const std::string& recv_account, const std::string message){
        try {
            // 获取数据库连接
            auto conn = _pool->getConnection();
            
            // 插入消息的 SQL 语句
            std::string sql = "INSERT INTO messages (sender_account, receiver_account, content) VALUES (?, ?, ?)";
            
            // 准备 SQL 语句
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            
            // 设置 SQL 语句的参数
            stmt->setString(1, send_account);
            stmt->setString(2, recv_account);
            stmt->setString(3, message);
            
            // 执行插入操作
            int rowsAffected = stmt->executeUpdate();
            
            // 释放数据库连接
            _pool->releaseConnection(conn);
            
            if (rowsAffected > 0) {
                return 0; // 消息插入成功
            } else {
                return -3; // 消息插入失败
            }
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1; // 其他错误
        }
    }
    std::vector<std::string> getMessage(const std::string& recv_account) {
        std::vector<std::string> messages; // 用于存储每条消息

        try {
            // 获取数据库连接
            auto conn = _pool->getConnection();
            
            // 查询消息的 SQL 语句
            std::string sql = "SELECT message_id, sender_account, content FROM messages WHERE receiver_account = ?";

            // 准备 SQL 语句
            std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
            
            // 设置 SQL 语句的参数
            stmt->setString(1, recv_account);
            
            // 执行查询操作
            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

            // 遍历查询结果
            while (res->next()) {
                int message_id = res->getInt("message_id");
                std::string sender_account = res->getString("sender_account");
                std::string content = res->getString("content");
                
                // 拼接消息内容
                std::string message = "[离线消息]From: " + sender_account + "\nMessage: " + content + "\n\n";
                
                // 将消息加入到结果 vector 中
                messages.push_back(message);
                
                // 删除已获取的消息
                std::string delete_sql = "DELETE FROM messages WHERE message_id = ?";
                std::unique_ptr<sql::PreparedStatement> delete_stmt(conn->prepareStatement(delete_sql));
                delete_stmt->setInt(1, message_id);
                delete_stmt->executeUpdate();
            }

            // 释放数据库连接
            _pool->releaseConnection(conn);

            return std::move(messages); // 返回所有消息
        } catch (sql::SQLException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return {}; // 返回空 vector，表示出错
        }
    }
private:
    // 生成随机盐
    std::string generateSalt(size_t length = 16) {
        unsigned char salt[length];
        if (RAND_bytes(salt, length) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }
        return std::string(reinterpret_cast<char*>(salt), length);
    }
    // 使用 PBKDF2 生成哈希
    std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32) {
        unsigned char hash[key_len];

        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                            reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                            iterations, EVP_sha256(), key_len, hash) != 1) {
            throw std::runtime_error("Failed to hash password");
        }

        return std::string(reinterpret_cast<char*>(hash), key_len);
    }
    // 将二进制数据转为十六进制字符串
    std::string toHex(const std::string& input) {
        std::ostringstream oss;
        for (unsigned char c : input) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
        }
        return oss.str();
    }
    std::string fromHex(const std::string& input) {
        std::string output;
        if (input.length() % 2 != 0) {
            throw std::invalid_argument("Invalid hex string");
        }

        for (size_t i = 0; i < input.length(); i += 2) {
            std::string byteStr = input.substr(i, 2);
            char byte = static_cast<char>(std::stoi(byteStr, nullptr, 16));
            output.push_back(byte);
        }

        return output;
    }
    // 校验密码
    bool verifyPassword(const std::string& password, const std::string& salt, const std::string& storedHash) {
        std::string newHash = hashPassword(password, salt);
        return newHash == storedHash;
    }

    MySQLConnectionPool* _pool;
};