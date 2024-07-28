系统编程大作业
=

# 题目：

一、 利用操作系统提供的系统调用、标准C库函数和POSIX 线程库pthread
函数设计并实现多人即时聊天软件。并利用学过的知识深入分析问题

![image](https://github.com/user-attachments/assets/f2863041-4e87-4e07-9bcd-ecb4c2f68451 "图 1.单机多用户聊天系统参考架构图")

## 1. 服务器初始化（40 分），见图 1，服务器是多线程守护进程。

1.1. 守护进程命名规则：chatserver_学生姓名拼音首字母缩写，例如，张三的守护进程命名为：chatserver_zs。<br><br>
1.2. 服务器配置文件设置以下信息：<br>
1.2.1. 众所周知的命名管道<br>
a) REG_FIFO ：命名为“学生姓名拼音首字母缩写+register”<br>
b) LOGIN_FIFO ：命名为“学生姓名拼音首字母缩写+login”<br>
c) MSG_FIFO ：命名为“学生姓名拼音首字母缩写+sendmsg”<br>
d) LOGOUT_FIFO ：命名为“学生姓名拼音首字母缩写+logout”<br>
1.2.2. 同时在线用户数最大值 MAX_ONLINE_USERS，该值为 4。同一用户同一时刻只允许在一个终端登录一次 MAX_LOG_PERUSER,该值为 1。\<br>
1.2.3. 用户日志文件存放目录 LOGFILES：/var/log/chat-logs/<br><br>
1.3. 服务器首先读取配置文件，通过配置文件约束行为。<br>
1.3.1. 根据配置文件创建众所周知的命名管道，采用多路复用技术<br>（select()、epoll()或 poll()）监听并读取以上信息，分别启动线程处理用户的注册请求、登录请求、聊天请求和退出请求，并将结果返回给用<br>户，并做日志记录。服务器要求运行期间忽略 SIGCHLD、SIGTERM 等信号。<br>
1.3.2. 为每个注册成功的用户建立独立日志文件，统一放在 1.2.3 设置的用户日志文件存放目录 LOGFILES 下。日志文件收集用户注册事件（用户名，注册，时间）、登录事件（用户名，登录，时间）、退出事件（用户名，退出，时间）、成功发送的信息（发送者，接收者，时间，true），未成功发送的信息（发送者，接收者，时间,false），日志文件只有超级用户可以读、写。同组用户和其他用户一律取消读、写、运行等权限。如果有未成功发送的信息，当接收者重新登录时服务器主动推送这些信息，并写相应日志文件（发送者，接收者，时间,true）。为安全起见，未成功发送的信息（发送者，接收者，信息，时间）应该存在特定的地方，更安全地保护起来（难点）。<br><br>

请在答卷完成以下内容： <br>
1） （10 分）请截图显示守护进程相关的进程信息、公共 FIFO 相关的信息，并解释各特征字段含义，截图显示命名管道创建、打开的代码。<br>
2） （10 分）请截图显示配置文件内容及服务器端相应的处理代码，服务器忽略 SIGCHLD、SIGTERM 信号的代码<br>
3） （10 分）请截图显示日志文件生成代码及有效日志文件属性及其内容，包括日志文件存放路径、权限，注册、登录、退出、成功发送消息和未成功发送消息事件记录的日志信息，未成功发送信息的管理等（难点）。<br>
4） （10 分）展示多路复用监听请求的代码、解释运行过程 select()、epoll()或 poll()的参数文件描述符集合的变化。特别提醒，代码质量将作为评分点。<br>

## 2. 基本聊天功能展示（40）：用户可以通过客户端注册、登录、收发信息和退出。

用户注册时提供用户名和密码，不允许不同用户使用同一用户名注册。客户端将为每个用户建立一个以用户名为名字的专用命名管道，用户A 发送给用户 B 的信息首先发送给服务器，然后服务器通过 B 的专用命名管道发送给用户 B。守护进程各线程通过共享内存共同维护和使用一个在线用户列表，包含用户名、密码及其私有命名管道。请注意其中的互斥访问和线程安全，请截图并讨论你的线程安全实现。

1） 截图用户注册的线程函数，截屏显示以下过程：启动 6 个终端，分别用以下用户名依次注册:学生姓名拼音_1、学生姓名拼音_2、 学生姓名拼音_3、最后学生姓名拼音_4、学生姓名拼音_5、学生姓名拼音_1。守护进程将注册不成功的信息返回给相应的用户。（8 分）。<br>
2） 截图客户端登录线程函数，截屏显示以下登录过程：启动 6 个终端，分别用以下用户名登录: 学生姓名拼音_1、学生姓名拼音_2、 学生姓名拼音_3、学生姓名拼音_4、学生姓名拼音_5、学生姓名拼音_3。守护进程将成功登录的用户信息及所有在线的总人数及用户名显示给所有的用户，不成功的信息仅返回给不成功的用户端。（12 分）名为“学生姓名拼音_2”的用户退出，守护进程将其退出的信息及剩余在线用户的信息显示给剩余的用户。保持名为“学生姓名拼音_2”用户的退出状态做 3)（4 分）。<br>
3） 截图用户消息发送过程，一对一发送：“学生姓名拼音_1”的用户给“学生姓名拼音_2”的用户发送信息“How are you，学生姓名拼音_2 ”，一对多发送：“学生姓名拼音_1”的用户给“学生姓名拼音_2”、“学生姓名拼音_3”的用户发送信息“Hi，let’s play badminton? ”，截屏三个用户的收发信息情况，包括成功和不成功的情况（可能较难）。最后名为“学生姓名拼音_2”的用户登录系统，收到前面的信息，展示发送的时间。（12分）。<br>
4） 截图讨论主线程收集新线程退出状态与否的代码实现，并讨论其优点和缺点（4 分）<br>

## 3. 基于线程池的多用户聊天服务器设计与实现（20 分）（难点）。

系统参考架构图见图 2，请结合代码介绍线程池的管理。

a) 在配置文件里新增线程池规模参数 POOLSIZE；值为 4；<br>
b) 服务器启动时根据配置文件新建 POOLSIZE 个线程，运行期接收请求时分派线程，请求处理完后回收线程用于再分派。请给出线程池管理数据结构设计、线程池线程状态管理算法设计和实现等，并附上相应的运 行 展 示 。 线 程 池 中 线 程 被 分 派 和 回 收 的 时 间 记 录 在 日 志/var/log/chat-logs/threads-log。截图展示日志信息。<br>

![image](https://github.com/user-attachments/assets/89e7ba40-8f88-4da2-a42b-beedd88b42c2 "图 2.基于线程池的单机多用户聊天系统参考架构图
")

# 二、 （难）openEuler 开源实习官网，领取一项任务并完成，贡献社区后提交任务报告：（https://www.openeuler.org/zh/internship/）

<br>
<br>
<br>

***

<br>
<br>
<br>

# 个人实现 - 多人即时聊天软件：

## 1.服务器初始化（40分）

### 1）（10分）请截图显示守护进程相关的进程信息、公共FIFO相关的信息，并解释各特征字段含义，截图显示命名管道创建、打开的代码。

PPID为1，则说明该进程是守护进程：

![image](https://github.com/user-attachments/assets/8c26644f-8169-4225-9c33-ffdea6cb1123)

这四个黄色的文件就是公共FIFO:

![image](https://github.com/user-attachments/assets/f595de05-ac1a-4bbd-b64b-553fce188a48)

命名管道名称宏定义在头文件info.h中：

![image](https://github.com/user-attachments/assets/2695db0e-7ea9-4af7-85e3-59adae82bba3)

命名管道创建：

![image](https://github.com/user-attachments/assets/3b0f70bc-861f-4e7a-bc96-9fd8dcca01f9)

命名管道的打开：

![image](https://github.com/user-attachments/assets/00b4533d-e901-4df4-a67f-e5ae5dcbfe2a)

### 2）（10分）请截图显示配置文件内容及服务器端相应的处理代码，服务器忽略SIGCHLD、SIGTERM信号的代码

info.h:

![image](https://github.com/user-attachments/assets/c71dfbe7-8708-40c3-a877-db0e887ba759)

Chatserver_wyx(服务器端)配置：

依次是 参数,主函数内的守护进程创建

![image](https://github.com/user-attachments/assets/29f6073b-b6e4-4014-b3d9-cc075a119dd2)

![image](https://github.com/user-attachments/assets/f8e8b56b-cbcd-45c5-a3be-a3a266b1539b)

### 3）（10分）请截图显示日志文件生成代码及有效日志文件属性及其内容，包括日志文件存放路径、权限，注册、登录、退出、成功发送消息和未成功发送消息的日志信息，未成功发送信息的管理等（难点）。

日志文件权限：

![image](https://github.com/user-attachments/assets/069e2a96-d506-4c09-a74e-593089817085)

注册：

![image](https://github.com/user-attachments/assets/2099ec29-b51a-4efa-8a4c-ee52fca4df97)

登录：

![image](https://github.com/user-attachments/assets/7073ea5b-73fd-4248-91c8-ccbe2650ceec)

退出：

![image](https://github.com/user-attachments/assets/8cbfd5ef-985b-4522-851c-c858385530bc)

成功发送消息：

![image](https://github.com/user-attachments/assets/29c9e1b9-4b2e-4bc0-ab64-824f69128c25)

未成功发送消息：

![image](https://github.com/user-attachments/assets/a4595959-541b-456d-a65f-3806c8b8a0a1)

未成功发送消息会给客户端的用户提示：

![image](https://github.com/user-attachments/assets/cb4773bf-d1ad-4529-ae4b-9c4d64a7ef9c)

### 4）（10分）请截图展示多路复用监听请求的代码、解释运行过程select()、epoll()或poll()的参数文件描述符集合的变化等，温馨提示：代码质量将作为评分点。

Server端:

![image](https://github.com/user-attachments/assets/a7a39af3-9c95-4e05-9a2f-05346a16727c)

![image](https://github.com/user-attachments/assets/8a2c952b-869d-4153-8358-e362a6d54ef8)

fd_set  readFds即为文件描述符集合，FD_ZERO初始化文件描述符集合，FD_SET需要监听的管道的文件描述符。select阻塞监听，直到有管道有内容可读时，对应文件描述符集合的元素被置标记，往下执行，多个if来确定哪个文件描述符被标记，然后创建线程进行相应操作，主程序初始化后接着监听。

Client端：

![image](https://github.com/user-attachments/assets/7f7901ac-8c3c-45e3-b36b-c052f95f9f9e)

## 2.	基本聊天功能展示（40分）

### 1）	截图用户注册的线程函数，启动6个终端用分别注册以下6个用户名，并截图展示运行细节（8分）<br>学生姓名拼音_1<br>学生姓名拼音_2<br>学生姓名拼音_3<br>学生姓名拼音_4<br>学生姓名拼音_5<br>学生姓名拼音_1<br>

![image](https://github.com/user-attachments/assets/1c9e7717-1b05-46b8-8cb1-4cee3001675e)

1~5注册是类似的

![image](https://github.com/user-attachments/assets/c471080f-52ee-45e7-83d6-69a669eafdc5)

再次注册1号会注册失败，并显示原因：

![image](https://github.com/user-attachments/assets/58d19502-e8bd-4cb3-bc44-773271ce9635)

### 2）	截图客户端登录线程函数，启动6个终端用分别注册以下6个用户名，并截图展示运行细节（12分）<br>学生姓名拼音_1<br>学生姓名拼音_2<br>学生姓名拼音_3<br>学生姓名拼音_4<br>学生姓名拼音_5<br>学生姓名拼音_3。<br>学生姓名拼音_2用户退出系统。（4分）<br>

server端的客户端登录线程函数：

![image](https://github.com/user-attachments/assets/83b61460-9533-4c6a-8806-57e2137f4eca)

![image](https://github.com/user-attachments/assets/2c8d2c71-a522-4fc0-8c61-a782f2c69d11)

![image](https://github.com/user-attachments/assets/4fd2be5e-a19d-4cff-a0b2-a97dd7d2f63d)

登录方法：

![image](https://github.com/user-attachments/assets/23cdb2a8-6fde-420a-98da-c2933e4b3db4)

随着其他用户登录或退出，已登录的1号会得到消息：

![image](https://github.com/user-attachments/assets/4a45c401-2550-42d5-8cb8-d62d9d52335e)

3号示例：

![image](https://github.com/user-attachments/assets/bb56e0f8-0f05-4dc7-bc55-20c05ac59958)

2号：

![image](https://github.com/user-attachments/assets/8bc589f9-3075-4e2b-8128-9c30f505fe64)

终端6登录失败（先检测了在线人数，所以显示在线人满了）：

![image](https://github.com/user-attachments/assets/9bffbe45-8618-460e-adf6-051a8915db0f)

### 3）	截图客户端发送信息函数，并用以下发送信息序列显示系统运行信息（12分）<br>“学生姓名拼音_1”的用户给“学生姓名拼音_2”的用户发送信息“How are you”，消息被挂起；<br>“学生姓名拼音_1”的用户给“学生姓名拼音_2”、“学生姓名拼音_3”的用户发送信息“Hi，let’s play badminton? ”，截屏三个用户的收发信息情况；<br>

学生姓名拼音_2登录后收到信息
发：

![image](https://github.com/user-attachments/assets/2af791c6-00ca-4d9a-b42c-e2f14da533bf)

![image](https://github.com/user-attachments/assets/d94ed61a-0b36-41bc-a567-726dfd4f9360)

![image](https://github.com/user-attachments/assets/fd6fef50-2e6b-4b41-8d6e-b3c117dddda7)

3号:

![image](https://github.com/user-attachments/assets/fe35ff4a-4b27-46eb-88c0-82390129f886)

### 4）	截图讨论主线程收集新线程退出状态与否的代码实现，并讨论其优点和缺点（4分）

## 3.	基于线程池的多用户聊天服务器设计与实现（20分）（难点，温馨提示：数据结构设计、算法设计及编码将作为评分点。）<br>（<br>请图文并茂展示线程池管理的代码和相应的运行结果，包括配置文件、线程池数据结构、管理算法（线程分派和回收）、线程分派和回收日志信息等。<br>）<br>特别提醒：代码质量和是否按时提交是本题评分点之一，请结合程序的正确性、线程的安全性、用户信息的安全性等讨论，迟交的同学本题分数直接减分，减分范围0~20分，以确保总评成绩不为A+为准。即如果该生整体成绩为A,虽然迟交，不扣分。如果该学生平时成绩基本满分，期末答卷也接近满分，但迟交，则本题将扣至该生不拿A+为止。<br>


