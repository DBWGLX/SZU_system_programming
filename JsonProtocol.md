### 通用字段定义
```
type：
类型：整数
含义：用于标识 JSON 报文的类型，具体取值及含义如下：
0：系统报文，例如系统广播消息、系统状态通知等。
1：注册报文，用户进行账号注册时发送的报文。
2：登录报文，用户进行账号登录时发送的报文。
3：获取在线人数报文，用于查询当前在线用户数量的请求报文。
4：聊天报文，用户之间进行聊天时发送的消息报文。
5：下线报文，用户主动退出登录或因其他原因下线时发送的报文。
6：修改个人信息报文，用户修改自己的个人信息时发送的报文。

account：
类型：字符串
含义：用户用于登录和标识身份的账号，具有唯一性。

token：
类型：字符串
含义：用户登录成功后服务器返回的身份令牌，用于后续请求的身份验证，每次请求时携带该令牌以证明用户身份。
```

### 不同报文类型

#### 0.回应报文

```
{
    "type": 0,
    "sequence": 1000,
    "status": "ACK" / "TIME_OUT"
}
```

#### 1.注册报文

```
{
    "type": 1,
    "sequence": 1000,
    "account": "test_account",
    "password": "test_password",
    "username": "Test User",
    "phone_number": "13812345678",
    "email": "test@example.com"
}
```

#### 2.登录报文

```
{
    "type": 2,
    "sequence": 1000,
    "account": "test_account",
    "password": "test_password"
}
```
返回一个token令牌

#### 3.获取在线人员

```
{
    "type": 3,
    "sequence": 1000,
    "account": "test_account",
    "token": "valid_token"
}
```

#### 4.聊天报文

```
{
    "type": 4,
    "sequence": 1000,
    "account": "sender_account",
    "token": "valid_token",
    "receiver_username": "receiver_username",
    "message": "Hello, how are you?"
}
```

#### 5.下线报文

```
{
    "type": 5,
    "sequence": 1000,
    "account": "test_account",
    "token": "valid_token"
}
```

#### 6.修改个人信息报文

```
{
    "type": 6,
    "sequence": 1000,
    "account": "test_account",
    "token": "valid_token",
    "phone_number": "13987654321",
    "username": "New Name"
}
```