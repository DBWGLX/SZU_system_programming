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
            stmt->setString(1,new_user.getAccount());
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
    // 校验密码
    bool verifyPassword(const std::string& password, const std::string& salt, const std::string& storedHash) {
        std::string newHash = hashPassword(password, salt);
        return newHash == storedHash;
    }

    MySQLConnectionPool* _pool;
};