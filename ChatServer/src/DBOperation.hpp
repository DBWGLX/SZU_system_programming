#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include "User.hpp"
#include "Logger.hpp"
#include "PasswordUtils.hpp"

class DBOperation {
public:
    DBOperation(std::shared_ptr<sql::Connection> conn);
    int addUser(const User& new_user);
    std::string getUsername(const std::string& account);
    bool verifyUser(const std::string& account, const std::string& inputPassword);
    int updateUserInfo(const User& user);
    int deleteUser(const std::string& account);
    int addMessage(const std::string& send_account, const std::string& recv_account, const std::string message);
    std::vector<std::string> getMessage(const std::string& recv_account);
    int deleteMessage(const std::string& recv_account);
private:
    bool verifyPassword(const std::string& password, const std::string& salt, const std::string& storedHash);

    std::shared_ptr<sql::Connection> _conn;
    //MySQLConnectionPool* _pool;
};
