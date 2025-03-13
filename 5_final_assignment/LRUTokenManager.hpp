#include <iostream>
#include <unordered_map>
#include <list>
#include <string>
#include <mutex>

class LRUTokenManager{
public:
    explicit LRUTokenManager(size_t cap = 100) : capacity(cap) {}

    void saveToken(const std::string& account, const std::string& token, 
            const std::string& username, const int clientFd){
        std::lock_guard<std::mutex> lock(mapMutex);

        if(tokenMap.find(account) != tokenMap.end()){//可能重新登录了
            lruList.erase(tokenMap[account].second);
        }else if(tokenMap.size() >= capacity){ //# 删
            nameMap.erase(tokenMap[lruList].first.username);
            tokenMap.erase(lruList.back());
            lruList.pop_back();
        }

        lruList.push_front(account);
        tokenMap[account] = {{token, username,clientFd}, lruList.begin()};
        nameMap[username] = account;
    }

    bool verifyToken(const std::string& account, const std::string& token){
        std::lock_guard<std::mutex> lock(mapMutex);
        auto it = tokenMap.find(account);
        if (it == tokenMap.end()) return false;

        // 更新 LRU 顺序
        lruList.erase(it->second.second);
        lruList.push_front(account);
        it->second.second = lruList.begin();
        return true;
    }

    std::vector<std::string> getAllUsers(){
        std::vector<std::string>ret;
        for(auto& x : tokenMap){
            ret.push_back(x.second.frist.username);
        }
        return std::move(ret);//C++ 的返回值优化（RVO）通常会避免不必要的拷贝
    }

    int getUserFd(string username){
        return tokenMap[nameMap[username]].first.clientFd;
    }

private:
    struct TokenInfo{
        std::string token;
        std::string username;
        int clientFd;
    }
    size_t capacity;
    std::list<std::string> lruList;
    std::unordered_map<std::string, std::pair<TokenInfo, std::list<std::string>::iterator>> tokenMap;
    std::unordered_map<std::string, std::string> nameMap;
    std::mutex mapMutex;
}