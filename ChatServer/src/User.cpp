#include "User.hpp"

// 默认构造函数
User::User() : _user_id(0) {}

// 带有用户名和密码的构造函数
User::User(const std::string& account, const std::string& username, const std::string& password, 
           const std::string& phone_number, const std::string& email)
    : _account(account), _username(username), _password(password), 
      _phone_number(phone_number), _email(email) {}

// 完整信息的构造函数
User::User(size_t user_id, const std::string& account, const std::string& username, 
           const std::string& salt, const std::string& password, const std::string& phone_number,
           const std::string& email, const std::string& create_time, const std::string& last_modify_time)
    : _user_id(user_id), _account(account), _username(username), _salt(salt),
      _password(password), _phone_number(phone_number), _email(email),
      _create_time(create_time), _last_modify_time(last_modify_time) {}

// Getter 方法
size_t User::getUserID() const { return _user_id; }
std::string User::getAccount() const { return _account; }
std::string User::getPassword() const { return _password; }
std::string User::getSalt() const { return _salt; }
std::string User::getUsername() const { return _username; }
std::string User::getPhoneNumber() const { return _phone_number; }
std::string User::getEmail() const { return _email; }
std::string User::getCreateTime() const { return _create_time; }
std::string User::getLastModifyTime() const { return _last_modify_time; }

// Setter 方法
void User::setUserID(size_t user_id) { _user_id = user_id; }
void User::setAccount(const std::string& account) { _account = account; }
void User::setPassword(const std::string& password) { _password = password; }
void User::setSalt(const std::string& salt) { _salt = salt; }
void User::setUsername(const std::string& username) { _username = username; }
void User::setPhoneNumber(const std::string& phone_number) { _phone_number = phone_number; }
void User::setEmail(const std::string& email) { _email = email; }
void User::setCreateTime(const std::string& create_time) { _create_time = create_time; }
void User::setLastModifyTime(const std::string& last_modify_time) { _last_modify_time = last_modify_time; }
