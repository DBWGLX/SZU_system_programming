#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "MySQLConnectionPool.hpp"
#include "User.hpp"
#include "Logger.hpp"

class DBOperation {
public:
    DBOperation(MySQLConnectionPool* pool);
    int addUser(const User& new_user);
    std::string getUsername(const std::string& account);
    bool verifyUser(const std::string& account, const std::string& inputPassword);
    int updateUserInfo(const User& user);
    int deleteUser(const std::string& account);
    int addMessage(const std::string& send_account, const std::string& recv_account, const std::string message);
    std::vector<std::string> getMessage(const std::string& recv_account);

private:
    std::string generateSalt(size_t length = 16);
    std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32);
    std::string toHex(const std::string& input);
    std::string fromHex(const std::string& input);
    bool verifyPassword(const std::string& password, const std::string& salt, const std::string& storedHash);

    MySQLConnectionPool* _pool;
};
