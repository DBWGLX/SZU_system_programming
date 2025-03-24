#include "DBOperation.hpp"

DBOperation::DBOperation(MySQLConnectionPool* pool)
    : _pool(pool) {}

int DBOperation::addUser(const User& new_user) {
    try {
        auto conn = _pool->getConnection();
        std::string sqlSentence = "INSERT INTO users (account, password, salt, username, phone_number, email) VALUES (?, ?, ?, ?, ?, ?)";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sqlSentence));

        std::string salt = generateSalt();
        std::string hashed = hashPassword(new_user.getPassword(), salt);

        stmt->setString(1, new_user.getAccount());
        stmt->setString(2, toHex(hashed));
        stmt->setString(3, toHex(salt));
        stmt->setString(4, new_user.getUsername());
        stmt->setString(5, new_user.getPhoneNumber());
        stmt->setString(6, new_user.getEmail());
        
        debug_str("new_user.email: "+new_user.getEmail());

        stmt->executeUpdate();
        _pool->releaseConnection(conn);

        return 0;
    } catch (sql::SQLException& e) {
        std::ostringstream oss;
        if(e.getErrorCode() == 1062) {
            oss << "Error: Duplicate entry error. Error code: " << e.getErrorCode() 
                << ", SQLState: " << e.getSQLState() 
                << ", Message: " << e.what();
            fatal_str(oss.str());
            return -1;
        }
        oss << "Error: General database error. Error code: " << e.getErrorCode() 
            << ", SQLState: " << e.getSQLState() 
            << ", Message: " << e.what() << std::endl;
        fatal_str(oss.str());
        return -2;
    }
}

std::string DBOperation::getUsername(const std::string& account) {
    try {
        auto conn = _pool->getConnection();
        std::string sqlSentence = "SELECT username FROM users WHERE account = ?";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sqlSentence));

        stmt->setString(1, account);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) {
            std::string username = res->getString("username");
            _pool->releaseConnection(conn);
            return username;
        } else {
            _pool->releaseConnection(conn);
            return "";  // 未找到账号
        }
    } catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        fatal_str(e.what());
        return "";  // 查询失败
    }
}


bool DBOperation::verifyUser(const std::string& account, const std::string& inputPassword) {
    try {
        auto conn = _pool->getConnection();
        std::string sql = "SELECT password, salt FROM users WHERE account = ?";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
        stmt->setString(1, account);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        if (res->next()) {
            std::string storedPassword = res->getString("password");
            std::string storedSalt = res->getString("salt");
            return verifyPassword(inputPassword, fromHex(storedSalt), fromHex(storedPassword));
        }
    } catch (sql::SQLException& e) {
        std::cerr << "Query error: " << e.what() << std::endl;
        fatal_str(e.what());
    }
    return false;
}

int DBOperation::updateUserInfo(const User& user) {
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

int DBOperation::deleteUser(const std::string& account) {
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
        return -1;
    }
}

int DBOperation::addMessage(const std::string& send_account, const std::string& recv_account, const std::string message) {
    try {
        auto conn = _pool->getConnection();
        std::string sql = "INSERT INTO messages (sender_account, receiver_account, content) VALUES (?, ?, ?)";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));

        stmt->setString(1, send_account);
        stmt->setString(2, recv_account);
        stmt->setString(3, message);

        int rowsAffected = stmt->executeUpdate();
        _pool->releaseConnection(conn);

        if (rowsAffected > 0) {
            return 0; // 消息插入成功
        } else {
            return -3; // 消息插入失败
        }
    } catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

std::vector<std::string> DBOperation::getMessage(const std::string& recv_account) {
    std::vector<std::string> messages;

    try {
        auto conn = _pool->getConnection();
        std::string sql = "SELECT message_id, sender_account, content FROM messages WHERE receiver_account = ?";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
        stmt->setString(1, recv_account);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        while (res->next()) {
            //int message_id = res->getInt("message_id");
            std::string sender_account = res->getString("sender_account");
            std::string content = res->getString("content");

            std::string message = "[离线消息]From: " + sender_account + "\nMessage: " + content + "\n\n";
            messages.push_back(message);
        }

        _pool->releaseConnection(conn);
        return std::move(messages);
    } catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return {};
    }
}

int DBOperation::deleteMessage(const std::string& recv_account){
    try {
        auto conn = _pool->getConnection();
        std::string sql = "DELETE FROM messages WHERE receiver_account = ?";
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(sql));
        stmt->setString(1, recv_account);
        int affectedRows = stmt->executeUpdate();

        _pool->releaseConnection(conn);
        return affectedRows;
    } catch (sql::SQLException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

std::string DBOperation::generateSalt(size_t length) {
    unsigned char salt[length];
    if (RAND_bytes(salt, length) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    return std::string(reinterpret_cast<char*>(salt), length);
}

std::string DBOperation::hashPassword(const std::string& password, const std::string& salt, int iterations, size_t key_len) {
    unsigned char hash[key_len];

    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                          reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                          iterations, EVP_sha256(), key_len, hash) != 1) {
        throw std::runtime_error("Failed to hash password");
    }

    return std::string(reinterpret_cast<char*>(hash), key_len);
}

std::string DBOperation::toHex(const std::string& input) {
    std::ostringstream oss;
    for (unsigned char c : input) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string DBOperation::fromHex(const std::string& input) {
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

bool DBOperation::verifyPassword(const std::string& password, const std::string& salt, const std::string& storedHash) {
    std::string newHash = hashPassword(password, salt);
    return newHash == storedHash;
}
