#include "PasswordUtils.hpp"

// 密码加密处理方法的定义
std::string generateSalt(size_t length) {
    unsigned char salt[length];
    if (RAND_bytes(salt, length) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }
    return std::string(reinterpret_cast<char*>(salt), length);
}

std::string hashPassword(const std::string& password, const std::string& salt, int iterations, size_t key_len) {
    unsigned char hash[key_len];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                        reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size(),
                        iterations, EVP_sha256(), key_len, hash) != 1) {
        throw std::runtime_error("Failed to hash password");
    }
    return std::string(reinterpret_cast<char*>(hash), key_len);
}

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