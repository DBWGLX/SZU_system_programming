# 服务器
chatserver_wyx.cpp 为main




### 优化

* io_uring 没有利用好提交队列

* 可以有初始化类，组织各个组件的初始化（而不是把数据库连接放入线程池中

服务启动可异步初始化


### 知识

stream是什么

move 会自动释放旧内存