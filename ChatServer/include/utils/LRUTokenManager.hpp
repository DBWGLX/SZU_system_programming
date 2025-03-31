// LRUTokenManager.h
#ifndef LRU_TOKEN_MANAGER_H
#define LRU_TOKEN_MANAGER_H

#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>

#define MAX_USERS_NUM 1e4

class LRUTokenManager {
public:
    explicit LRUTokenManager(size_t cap = MAX_USERS_NUM);
    std::string generate_token(size_t length = 32);
    void saveToken(const std::string& account, const std::string& token, const std::string& username, int clientFd);
    bool verifyToken(const std::string& account, const std::string& token);
    std::vector<std::pair<std::string, std::string>> getAllUsers();
    int getUserFd(const std::string& account);
    std::string getUserName(const std::string& account);
    bool logout(const std::string& account);

private:
    struct TokenInfo {
        std::string token;
        std::string username;
        int clientFd;
    };

    size_t capacity;
    std::list<std::string> lruList;
    std::unordered_map<std::string, std::pair<TokenInfo, std::list<std::string>::iterator>> tokenMap;
    std::mutex mapMutex;
};

#endif // LRU_TOKEN_MANAGER_H
