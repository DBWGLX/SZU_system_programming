#pragma once
#include <string>

class User {
public:
    // 默认构造函数
    User();

    // 带有用户名和密码的构造函数
    User(const std::string& account, const std::string& password, const std::string& username, 
         const std::string& salt, const std::string& phone_number = "", const std::string& email = "");

    // 完整信息的构造函数
    User(size_t user_id, const std::string& account, const std::string& password, 
         const std::string& salt, const std::string& username, const std::string& phone_number,
         const std::string& email, const std::string& create_time,
         const std::string& last_modify_time);

    // Getter 方法
    size_t getUserID() const;
    std::string getAccount() const;
    std::string getPassword() const;
    std::string getSalt() const;
    std::string getUsername() const;
    std::string getPhoneNumber() const;
    std::string getEmail() const;
    std::string getCreateTime() const;
    std::string getLastModifyTime() const;

    // Setter 方法
    void setUserID(size_t user_id);
    void setAccount(const std::string& account);
    void setPassword(const std::string& password);
    void setSalt(const std::string& password);
    void setUsername(const std::string& username);
    void setPhoneNumber(const std::string& phone_number);
    void setEmail(const std::string& email);
    void setCreateTime(const std::string& create_time);
    void setLastModifyTime(const std::string& last_modify_time);

private:
    size_t _user_id;
    std::string _account;
    std::string _password;
    std::string _salt;
    std::string _username;
    std::string _phone_number;
    std::string _email;
    std::string _create_time;
    std::string _last_modify_time;
};
