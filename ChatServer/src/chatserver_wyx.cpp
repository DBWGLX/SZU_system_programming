/*
    服务器启动程序
    初始化：监听服务，消费者，守护进程

*/
#include <iostream>
#include <csignal>
#include <stdexcept>
#include <atomic>
#include <unistd.h>
#include "Logger.hpp"
#include "EpollServer.hpp"

//程序中断全局变量
std::atomic<bool> interrupted(false);//原子变量可以保证在多线程环境下的安全访问。
void signalHandler(int signum){
    if(signum == SIGINT){//Ctrl + C
        interrupted = true;
    }
}

// 守护进程 ：创建新会话并成为会话首进程
void init(){
    pid_t pid;
    pid = fork();
    if (pid < 0) {  perror("Fork failed");fflush(NULL);exit(1);}
    if (pid > 0) {  exit(0); }
    if (setsid() < 0) {
        perror("setsid error");
        exit(1);
    }

    /////
    signal(SIGINT, signalHandler);
}

int main(){
    //1.尝试启动为守护进程，初始化后持续运行监听服务
    try{
        printf("\033[33m#### Welcome to use the chat server.The server will run.\n");
        printf("#### The author is DBWGLX.Learn more in https://github.com/DBWGLX/SZU_system_programming. Thank you!🤓❤️\n\033[0m");

        init();
        info_str("🟢 服务器启动");
        LOG("🟢 服务器启动");
        EpollServer eServer(interrupted);
        eServer.work();

    } catch(const std::exception& e){
        std::cerr << "Caught exception: " << e.what() << std::endl;
        fatal_str(e.what());
        return 1;
    }

    //2.服务器停止
    fatal_str("🛑 Service terminated.");
    LOG("🛑 Service terminated.");
    sleep(1);
    return 0;
}

