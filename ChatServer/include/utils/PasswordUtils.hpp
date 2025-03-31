#include <string>
#include <openssl/evp.h>
#include <openssl/rand.h>

// 密码加密处理方法
std::string generateSalt(size_t length = 16);
std::string hashPassword(const std::string& password, const std::string& salt, int iterations = 10000, size_t key_len = 32);
std::string toHex(const std::string& input);
std::string fromHex(const std::string& input);
