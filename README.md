![](https://img.shields.io/github/repo-size/DBWGLX/SZU_system_programming.svg)
![](https://img.shields.io/github/languages/code-size/DBWGLX/SZU_system_programming.svg)
![](https://img.shields.io/github/license/DBWGLX/SZU_system_programming.svg)

基于 Linux 的 C++ 网络聊天服务器演示项目
=

### 项目概述

本项目是一个网络聊天系统的演示示例，包含聊天服务器与客户端两部分。该系统借助 Socket 进行 TCP 流式网络通信，能够实现多用户之间的实时聊天交互。

#### 服务器端特性

- 事件通知与任务处理：<br>服务器运用 **epoll** 机制来高效监测事件的发生。一旦有事件触发，会将对应的文件描述符加入任务队列。<br>为了提升处理效率，系统采用**线程池**对任务队列中的任务进行处理。线程池会接收 **JSON 格式**的报文，解析其中的数据，并依据报文中 type 字段的不同，调用相应的处理方法，最后将处理结果发送回对应的文件描述符。

- 数据持久化存储与连接池实现：<br>服务器具备用户数据和离线消息的持久化存储能力。借助 **MySQL Connector/C++** 这一专业的数据库连接库，服务器能够轻松建立与 MySQL 数据库的连接，并执行各类数据操作。<br>为进一步优化数据库连接的使用效率和性能，服务器实现了**数据库连接池**， 避免了频繁创建和销毁数据库连接带来的开销，显著提升了系统的响应速度和并发处理能力，确保大量用户数据和离线消息能够高效、稳定地存储到数据库中。

- 用户密码安全处理，用户身份验证与 Token 机制：<br>为保障用户密码的安全性，服务器采用**加盐哈希**的方式对用户密码进行处理。借助 OpenSSL 库，生成随机盐值，来对用户密码进行哈希加密。<br>当用户登录成功后，服务器会为其生成一个唯一的 **token 令牌**。这个 token 同样借助 OpenSSL 库生成，具有较高的安全性。在后续的网络交互过程中，客户端需要携带该 token 与服务器进行通信。服务器通过验证账号和 token 的组合来确认用户的身份，确保只有合法用户能够进行相关操作，进一步增强了系统的安全性和可靠性。

- 在线用户管理：<br>服务器采用 **LRU（Least Recently Used，最近最少使用）算法**实现的数据结构来管理在线用户。该数据结构会储存登录用户账号信息和token令牌，便于后续身份核验，确保每一次交互都源自合法且活跃的用户。

- 日志管理与信号处理：<br>服务器使用了自主开发的**日志器**（可参考[https://github.com/DBWGLX/Cpp-Logging-Component-on-Independent-Threads](https://github.com/DBWGLX/Cpp-Logging-Component-on-Independent-Threads)），该日志器采用异步线程将日志信息写入文件，避免了日志记录操作对主线程的影响，提高了系统的性能。<br>服务器支持通过 Ctrl + C 发送的 2 号**信号**实现优雅退出。在接收到退出信号后，日志器会确保所有日志信息都完整地写入文件后，才会关闭服务器，保证了数据的完整性和一致性。

#### 客户端特性

- 类 Shell 操作界面：<br>客户端的操作界面类似于 Shell 命令行，会明确提示用户支持的操作。用户可以在客户端进行注册、登录、查看当前在线人数、发送消息以及退出系统等操作，操作简单直观。

- 双线程设计：<br>客户端采用双线程架构，一个线程专门负责接收服务器发送的消息，另一个线程则负责向服务器发送用户输入的消息。这种设计使得客户端能够在接收消息的同时，及时响应用户的输入，提供流畅的交互体验。

#### 其他设计

报文协议见 JsonProtocol.md

数据库设计见 DATABASE.md

### 效果展示

- 服务器的启动与退出<br>
![image](https://github.com/user-attachments/assets/4e397cc2-38bf-49e6-ace8-bbdca267966e)
![image](https://github.com/user-attachments/assets/e3345180-ea41-4408-a1db-717c4395c097)
![image](https://github.com/user-attachments/assets/f8409b7c-4b54-4c67-b456-d345a447e863)

- 客户端操作<br>
操作指南<br>
![image](https://github.com/user-attachments/assets/e4bbe743-31c4-440e-b168-9653d906a6fd)
<br>注册<br>
![image](https://github.com/user-attachments/assets/d377d1e2-c779-4fb1-83b1-3cc39b03e4a9)
<br>登录<br>
![image](https://github.com/user-attachments/assets/abf5d8e2-ddf3-4673-9c04-0ff901cfa41f)
<br>获取在线用户<br>
![image](https://github.com/user-attachments/assets/b4cd5ebb-09cf-4963-8d06-c44130d34c75)
<br>聊天<br>
![image](https://github.com/user-attachments/assets/ac7cdd26-052d-4703-872c-8cd7d8264466)
<br>退出<br>
![image](https://github.com/user-attachments/assets/4c974182-4ca8-456c-bf6b-fd680259d39e)

- 数据库展示<br>
![image](https://github.com/user-attachments/assets/75fd89b7-5334-4d7b-ba48-8adcc645f502)

- 日志展示<br>
![image](https://github.com/user-attachments/assets/f15a6e1c-83c8-48be-b877-5845e3ac9cc5)



