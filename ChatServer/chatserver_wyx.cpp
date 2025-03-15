#include <iostream>
#include <csignal>
#include <stdexcept>
#include <atomic>
#include <unistd.h>
#include "Logger.hpp"
#include "EpollServer.hpp"

//程序中断
std::atomic<bool> interrupted(false);//原子变量可以保证在多线程环境下的安全访问。
void signalHandler(int signum){
    if(signum == SIGINT){//Ctrl + C
        interrupted = true;
    }
}

void init(){
    // 守护进程 ：创建新会话并成为会话首进程
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
    try{
        printf("\033[33m#### Welcome to use the chat server.The server will run.\n");
        printf("#### The author is DBWGLX.Learn more in https://github.com/lubenweiNBNBNBNB. Thank you!🤓❤️\n\033[0m");

        init();
        info_str("🟢 服务器启动");
        LOG("\n🟢 服务器启动");
        EpollServer eServer;
        eServer.work(interrupted);

    } catch(const std::exception& e){
        std::cerr << "Caught exception: " << e.what() << std::endl;
        fatal_str(e.what());
        return 1;
    }

    return 0;
}

