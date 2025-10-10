#ifndef PASSWORD_UTILS_HPP
#define PASSWORD_UTILS_HPP
#include <string>
#include <stdexcept>  // 包含 std::runtime_error 定义
#include <sstream>    // 包含 std::ostringstream 定义
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>

// 密码加密处理方法
class PasswordUtils{
public:
    static std::string generateSalt(size_t length = 16);
    static std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32);
    static std::string toHex(const std::string& input);
    static std::string fromHex(const std::string& input);
};

#endif