聊天服务器数据库设计：
用户表：用户id 账号 密码 用户名 用户手机号 用户邮箱 


```
-- 创建用户表
CREATE TABLE users (
    -- 用户 ID，主键，自增
    user_id INT AUTO_INCREMENT COMMENT '用户的唯一标识，系统自动分配的 ID',
    -- 用户账号，唯一，非空
    account VARCHAR(50) NOT NULL UNIQUE COMMENT '用户用于登录的账号，必须唯一',
    -- 用户密码，非空
    password VARCHAR(255) NOT NULL COMMENT '用户登录时使用的密码',
    -- 用户姓名，唯一，非空
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户的真实姓名或昵称',
    -- 用户手机号，唯一，可空
    phone_number VARCHAR(20) UNIQUE COMMENT '用户的手机号码，可用于找回密码等操作',
    -- 用户邮箱，唯一，可空
    email VARCHAR(100) UNIQUE COMMENT '用户的电子邮箱地址，可用于接收通知等',
    -- 创建时间，默认值为当前时间
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '用户记录的创建时间',
    -- 最近修改时间，在记录更新时自动更新
    last_modify_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '用户记录的最近修改时间',
    -- 设置用户 ID 为主键
    PRIMARY KEY (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT '存储用户的基本信息，包括账号、密码、姓名、手机号、邮箱以及创建和修改时间等';

```