聊天服务器数据库设计：
用户表：用户id 账号 密码 用户名 用户手机号 用户邮箱 
聊天记录表：消息id 用户id 对方名称 消息内容

```
CREATE database chatServer;

-- 创建用户表
CREATE TABLE users (
    -- 用户 ID，主键，自增
    user_id INT AUTO_INCREMENT COMMENT '用户的唯一标识',
    -- 用户账号，唯一，非空
    account VARCHAR(50) NOT NULL UNIQUE COMMENT '用户账号',
    -- 用户密码，非空
    password VARCHAR(255) NOT NULL COMMENT '用户密码',
    -- 盐值，非空
    salt VARCHAR(255) NOT NULL COMMENT '用于加密密码的随机盐值',
    -- 用户姓名，唯一，非空
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    -- 用户手机号，唯一，可空
    phone_number VARCHAR(20) UNIQUE COMMENT '用户的手机号码',
    -- 用户邮箱，唯一，可空
    email VARCHAR(100) UNIQUE COMMENT '用户的电子邮箱地址',
    -- 创建时间，默认值为当前时间
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '用户记录的创建时间',
    -- 最近修改时间，在记录更新时自动更新
    last_modify_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '用户记录的最近修改时间',
    -- 设置用户 ID 为主键
    PRIMARY KEY (user_id),
    -- 创建 account 字段的索引
    INDEX idx_account (account)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT '用户的基本信息';


-- 创建消息表
CREATE TABLE messages (
    -- 消息 ID，主键，自增
    message_id INT AUTO_INCREMENT COMMENT '消息的唯一标识',
    -- 发送者用户 account，关联用户表
    sender_account VARCHAR(50) NOT NULL COMMENT '发送消息的用户账号',
    -- 接收者用户 account，关联用户表
    receiver_account VARCHAR(50) NOT NULL COMMENT '接收消息的用户账号',
    -- 消息内容，非空
    content TEXT NOT NULL COMMENT '聊天消息内容',
    -- 设置消息 ID 为主键
    PRIMARY KEY (message_id),
    INDEX idx_receiver (receiver_account),
    -- 建立发送者和接收者的外键关联
    FOREIGN KEY (sender_account) REFERENCES users(account) ON DELETE CASCADE,
    FOREIGN KEY (receiver_account) REFERENCES users(account) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT '离线消息';


```