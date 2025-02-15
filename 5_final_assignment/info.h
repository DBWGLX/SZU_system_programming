//这里是服务端和客户端共用的头文件。定义了传送信息的格式。
//以及一些函数kit

#ifndef _INFO_H
#define _INFO_H

#pragma once
#include <ctime>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <algorithm>
#include <iostream>
using namespace std;

//这是服务器的接收管道
#define FIFO_NAME1 "wyx_register"
#define FIFO_NAME2 "wyx_login"
#define FIFO_NAME3 "wyx_sendmsg"
#define FIFO_NAME4 "wyx_logout"
#define LOG_TXT "/var/log/chat-logs/LOGFILES"

//约定：客户端以用户名创建文件夹，里面放个人接收管道
//chat 接收聊天信息
//log 接受登录的结果代号


#define BUFF_SZ 150
#define MAX_ONLINE_USERS 4
#define MAX_LOG_PERUSER 1 //同一用户同一时刻只允许在一个终端登录一次

//数据包格式
typedef struct {
        char name[150];    /*client's FIFO name */
        char password[150];
        char myfifo[150];
        char touser[150];
        char context[150];
}CLIENTINFO, * CLIENTINFOPTR;

void printTime()
{
    time_t curr = time(nullptr);
    struct tm *tmp = localtime(&curr);
    printf("\n\n==== %d-%d-%d %d:%d:%d ====\n", tmp->tm_year + 1900, tmp->tm_mon+1, tmp->tm_mday,
             tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
}

string getTime()
{
    time_t curr = time(nullptr);
    struct tm *tmp = localtime(&curr);
    char buffer[128];
    sprintf(buffer,"%d-%d-%d %d:%d:%d", tmp->tm_year + 1900, tmp->tm_mon+1, tmp->tm_mday,
             tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    return buffer;
}

#endif

