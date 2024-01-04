#ifndef _INFO_H
#define _INFO_H

//#pragma once
#include<ctime>
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

#define FIFO_NAME1 "/home/wangyaxian2022150054/5_final_assignment/wyx_register"
#define FIFO_NAME2 "/home/wangyaxian2022150054/5_final_assignment/wyx_login"
#define FIFO_NAME3 "/home/wangyaxian2022150054/5_final_assignment/wyx_sendmsg"
#define FIFO_NAME4 "/home/wangyaxian2022150054/5_final_assignment/wyx_logout"
#define LOG_TXT "/var/log/chat-logs/LOGFILES"
//#define LOG_TXT "/home/wangyaxian2022150054/5_final_assignment/LOGFILES"
#define BUFF_SZ 150
#define MAX_ONLINE_USERS 4
#define MAX_LOG_PERUSER 1

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

