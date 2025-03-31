#include "LRUTokenManager.hpp"
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>

LRUTokenManager::LRUTokenManager(size_t cap) : capacity(cap) {}

std::string LRUTokenManager::generate_token(size_t length) {
    unsigned char buffer[32];
    if (RAND_bytes(buffer, sizeof(buffer)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    std::ostringstream token;
    for (size_t i = 0; i < length && i < sizeof(buffer); ++i) {
        token << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    return token.str();
}

void LRUTokenManager::saveToken(const std::string& account, const std::string& token, const std::string& username, int clientFd) {
    std::lock_guard<std::mutex> lock(mapMutex);

    if (tokenMap.find(account) != tokenMap.end()) {
        lruList.erase(tokenMap[account].second);
    } else if (tokenMap.size() >= capacity) {
        tokenMap.erase(lruList.back());
        lruList.pop_back();
    }

    lruList.push_front(account);
    tokenMap[account] = {{token, username, clientFd}, lruList.begin()};
}

bool LRUTokenManager::verifyToken(const std::string& account, const std::string& token) {
    std::lock_guard<std::mutex> lock(mapMutex);
    auto it = tokenMap.find(account);
    if (it == tokenMap.end()) return false;

    lruList.erase(it->second.second);
    lruList.push_front(account);
    it->second.second = lruList.begin();
    return true;
}

std::vector<std::pair<std::string, std::string>> LRUTokenManager::getAllUsers() {
    std::vector<std::pair<std::string, std::string>> ret;
    for (auto& x : tokenMap) {
        ret.push_back({x.second.first.username, x.first});
    }
    return ret;
}

int LRUTokenManager::getUserFd(const std::string& account) {
    if(tokenMap.count(account))
        return tokenMap[account].first.clientFd;
    return -1;
}

string LRUTokenManager::getUserName(const std::string& account) {
    if(tokenMap.count(account))
        return tokenMap[account].first.username;
    return "error";
}

bool LRUTokenManager::logout(const std::string& account) {
    if (tokenMap.find(account) != tokenMap.end()) {
        lruList.erase(tokenMap[account].second);
        tokenMap.erase(account);
    }
    return true;
}
