#pragma once
#include <string>

class User {
public:
    // 默认构造函数
    User() : _user_id(0) {}

    // 带有用户名和密码的构造函数
    User(const std::string& account, const std::string& password, const std::string& username, 
        const std::string& phone_number = "", const std::string& email = "")
        : _account(account), _username(username), _password(password), 
        _salt(salt), _phone_number(phone_number), _email(email) {
    }

    // 完整信息的构造函数
    User(size_t user_id, const std::string& account, const std::string& password, 
         const std::string& salt, const std::string& username, const std::string& phone_number,
         const std::string& email, const std::string& create_time,
         const std::string& last_modify_time)
        : _user_id(user_id), _account(account), _password(password),_salt(salt),
          _username(username), _phone_number(phone_number), _email(email),
          _create_time(create_time), _last_modify_time(last_modify_time) {}

    // Getter 方法
    size_t getUserID() const { return _user_id; }
    std::string getAccount() const { return _account; }
    std::string getPassword() const { return _password; }
    std::string getSalt() const { return _salt; }
    std::string getUsername() const { return _username; }
    std::string getPhoneNumber() const { return _phone_number; }
    std::string getEmail() const { return _email; }
    std::string getCreateTime() const { return _create_time; }
    std::string getLastModifyTime() const { return _last_modify_time; }

    // Setter 方法
    void setUserID(size_t user_id) { _user_id = user_id; }
    void setAccount(const std::string& account) { _account = account; }
    void setPassword(const std::string& password) { _password = password; }
    void setSalt(const std::string& password) { _password = password; }
    void setUsername(const std::string& username) { _username = username; }
    void setPhoneNumber(const std::string& phone_number) { _phone_number = phone_number; }
    void setEmail(const std::string& email) { _email = email; }
    void setCreateTime(const std::string& create_time) { _create_time = create_time; }
    void setLastModifyTime(const std::string& last_modify_time) { _last_modify_time = last_modify_time; }

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