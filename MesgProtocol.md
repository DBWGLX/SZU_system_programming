### 通用字段定义

K L V
格式 长度 数据

批量测试时发现粘包了，length是有必要的

### 不同报文类型

```
type：用于标识 JSON 报文的类型，具体取值及含义如下：
0：系统报文
1：注册报文
2：登录报文
3：获取在线人数报文
4：聊天报文
5：下线报文
6：修改个人信息报文
```

#### 服务器发送给客户端的消息一般格式

```
{
    "type": 1001,
    "message": "success"
}
```

#### 客户端发送与接收的报文

#### 1.注册报文

```
{
    "type": 1000,
    "account": "test_account",
    "password": "test_password",
    "username": "Test User",
    "phone_number": "13812345678",
    "email": "test@example.com"
}
```

1001 注册成功
1002 注册失败

#### 2.登录报文

```
{
    "type": 2000,
    "account": "test_account",
    "password": "test_password"
}
```

2001 登录成功，返回一个token令牌
```
{
    "type": 2001,
    "message": "3*7/$ay",
    "username": "用户名"
}
```
2002 登录失败

#### 3.获取在线人员

```
{
    "type": 3000,
    "account": "test_account",
    "token": "valid_token"
}
```

3001 获取成功，返回人员列表
```
{
    "type": 3001,
    "users": [
        {"name": "Alice", "account": "1001"},
        {"name": "Bob", "account": "1002"},
        {"name": "Charlie", "account": "1003"}
    ]
}

```
3002 获取失败

#### 4.聊天报文

```
{
    "type": 4000,
    "account": "sender_account",
    "token": "valid_token",
    "receiver_useraccount": "receiver_useraccount",
    "message": "Hello, how are you?"
}
```

4001 发送成功
4002 发送失败

4010 收到消息
```
{
    "type": 4010,
    "account": "sender_account",
    "name": "sender_name",
    "message": "Hello, how are you?"
}
```

#### 5.下线报文

```
{
    "type": 5000,
    "account": "test_account",
    "token": "valid_token"
}
```

5001 下线成功

#### 6.修改个人信息报文

```
{
    "type": 6000,
    "account": "test_account",
    "token": "valid_token",
    "phone_number": "13987654321",
    "username": "New Name"
}
```


