//测试哈希加盐加密
#include <iostream>
#include <iomanip>
#include <sstream>      // 用于 std::ostringstream
#include <stdexcept>    // 用于 std::runtime_error
#include <openssl/evp.h>
#include <openssl/rand.h>

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

int main() {
    try {
        std::string password = "MySecret123";
        std::string salt = generateSalt();
        std::string hashed = hashPassword(password, salt);

        std::cout << "Salt (hex): " << toHex(salt) << std::endl;
        std::cout << "Hashed Password (hex): " << toHex(hashed) << std::endl;

        // 模拟验证
        std::string inputPassword;
        std::cout << "输入密码进行验证: ";
        std::cin >> inputPassword;

        if (verifyPassword(inputPassword, salt, hashed)) {
            std::cout << "密码正确！" << std::endl;
        } else {
            std::cout << "密码错误！" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}


// #include<iostream>
// #include<unistd.h>
// //测试数据库
// #include "MySQLConnectionPool.hpp"
// #include <cppconn/exception.h>
// void threadFunction(MySQLConnectionPool& pool) {
//     try {
//         auto conn = pool.getConnection();
//         std::cout << "Connection acquired in thread: " << std::this_thread::get_id() << std::endl;

//         // Example of using the connection
//         std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement("SELECT 1"));
//         std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
//         while (res->next()) {
//             std::cout << "Query result: " << res->getInt(1) << std::endl;
//         }

//         pool.releaseConnection(conn);
//         std::cout << "Connection released in thread: " << std::this_thread::get_id() << std::endl;
//     } catch (sql::SQLException& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }

// int main() {
//     MySQLConnectionPool pool("tcp://127.0.0.1:3306", "root", "123456", "chatServer", 5);

//     // Spawn multiple threads to simulate concurrent database access
//     std::vector<std::thread> threads;
//     for (int i = 0; i < 300; ++i) {
//         threads.emplace_back(threadFunction, std::ref(pool));//ref是应对线程拷贝函数参数。
//     }

//     for (auto& t : threads) {
//         t.join();
//     }
//     return 0;
// }