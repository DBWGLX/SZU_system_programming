## MySQL驱动
find / -name "mysql_driver.h"
/usr/include/mysql-cppconn/jdbc/mysql_driver.h

# 1.c++17的支持：CentOS 切换GCC版本 Software Collections
scl enable devtoolset-8 bash  

# 2.运行参数
-std=c++17 
-I/usr/include/mysql-cppconn/jdbc 
-lmysqlcppconn 
-pthread 
-L /root/work/EpollChatServer/cmake_out/lib -lasync_logger 

# json jansson库
-ljansson
# openSSH 
-lcrypto
# io_uring 
-luring
# protoc
protoc --cpp_out=. chat.proto


# 3.运行问题

## ※ (链接dbwg的日志器) 临时设置终端环境变量 为动态库位置（非默认目录时）
export LD_LIBRARY_PATH=/home/wyx/work/Cpp-Logging-Component-on-Independent-Threads/cmake_out/lib:$LD_LIBRARY_PATH

#### 系统调试：设置更大的接收缓冲区  最大 和 默认 ；程序里面可以再去调整
sudo sysctl -w net.core.rmem_max=16777216
sudo sysctl -w net.core.rmem_default=8388608
sysctl -w net.core.somaxconn=10000
查看
sysctl net.core.rmem_max
sysctl net.core.rmem_default
sysctl net.core.somaxconn

##### MySQL: 清理数据表自增
delete from users;
ALTER TABLE users AUTO_INCREMENT = 1;  