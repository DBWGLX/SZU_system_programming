#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "info.h"
#include <signal.h>
#include <string.h>
#include <errno.h>

#define FIFO_NAME1 "/home/wangyaxian2022150054/2_experiment_Inter-Process_Communication/task4/FIFO_1"
#define FIFO_NAME2 "/home/wangyaxian2022150054/2_experiment_Inter-Process_Communication/task4/FIFO_2"
#define FIFO_NAME3 "/home/wangyaxian2022150054/2_experiment_Inter-Process_Communication/task4/FIFO_3"
#define BUFF_SZ 150

//void handler(int sig){
//    unlink(FIFO_NAME);
//    exit(1);
//}

CLIENTINFO cookies[150];
int cookiesNum = 0;

int main(){
    int res;
    int i;
    int fifo_fd1,fifo_fd2,fifo_fd3,fdl;
    CLIENTINFO info;
    char buffer[150];
    memset(cookies,0,sizeof cookies);
    //signal(SIGKILL, handler);
    //signal(SIGINT, handler);
    //signal(SIGTERM, handler);


    //printf("#### Check info first!\n");
    //printf("#### sizeof CLIENTINFO:%d\n",sizeof(CLIENTINFO));
    //printf("#### sizeof info:%d\n",sizeof(info));

    /* create FIFO, if necessary */
    //建立管道
    if (access(FIFO_NAME1, F_OK) == -1) {
        res = mkfifo(FIFO_NAME1, 0777);
        if(res != 0) {
            printf("FIFO1 %s was not created\n", FIFO_NAME1);
            exit(EXIT_FAILURE);
        }
    }
    if (access(FIFO_NAME2, F_OK) == -1) {
        res = mkfifo(FIFO_NAME2, 0777);
        if(res != 0) {
            printf("FIFO2 %s was not created\n", FIFO_NAME2);
            exit(EXIT_FAILURE);
        }
    }
    if (access(FIFO_NAME3, F_OK) == -1) {
        res = mkfifo(FIFO_NAME3, 0777);
        if(res != 0) {
            printf("FIFO3 %s was not created\n", FIFO_NAME3);
            exit(EXIT_FAILURE);
        }
    }
    /* create FIFO, if necessary */


    printf("\nServer is rarin' to go!\n");


    //非阻塞读三个管道
    //while(1)
    //{
        

        /* open FIFO for reading *///非阻塞读，三个管道轮着读
        //分别打开三个管道
        fifo_fd1 = open(FIFO_NAME1, O_RDWR|O_NONBLOCK);
        if(fifo_fd1 == -1){
            printf("Could not open %s for read only access\n",
                    FIFO_NAME1);
            exit(EXIT_FAILURE);
        }
        fifo_fd2 = open(FIFO_NAME2, O_RDWR|O_NONBLOCK);
        if(fifo_fd2 == -1){
            printf("Could not open %s for read only access\n",
                    FIFO_NAME2);
            exit(EXIT_FAILURE);
        }
        fifo_fd3 = open(FIFO_NAME3, O_RDWR|O_NONBLOCK);
        if(fifo_fd3 == -1){
            printf("Could not open %s for read only access\n",
                    FIFO_NAME3);
            exit(EXIT_FAILURE);
        }
        ///

    while(1)
    {
        res = read(fifo_fd1, &info, sizeof(CLIENTINFO));    //1.读注册信息 info
        if (res != -1){
            printf("1.Client'register arrived!!res:%d\n",res);
            printf("#### sizeof info:%d\n",sizeof(info));
            printf("#### info.name:%s\n",info.name);
            printf("#### info.password:%s\n",info.password);
            printf("#### info.myfifo:%s\n",info.myfifo);
            int flag = 1;
            for(int i=0;i<cookiesNum;i++)   //检查是否有注册
            {
                if(!strcmp(cookies[i].name,info.name))
                {
                    flag = 0;break;
                }
            }
            if(flag)
            {
                sprintf(buffer,"1");    //可以注册
                strcpy(cookies[cookiesNum].name,info.name);
                strcpy(cookies[cookiesNum].password,info.password);
                strcpy(cookies[cookiesNum].myfifo,info.myfifo);
                cookiesNum++;
                printf("#### Register successes!\n");
                printf("#### Now,we have %d users:\n",cookiesNum);
                for(int j=0;j<cookiesNum;j++)
                {
                    printf("####     %d.%s\n",j,cookies[j].name);
                }
            }
            else
            {
                sprintf(buffer,"0");
                printf("#### Register failed!\n");
            }
            int fdl = open(info.myfifo, O_WRONLY);
            write(fdl, buffer, strlen(buffer)+1);
            printf("#### have fed back!\n\n");
            close(fdl);
        }

        res = read(fifo_fd2, &info, sizeof(CLIENTINFO));    //2.读登录信息
        if (res != -1){
            printf("2.Client'login arrived!!res:%d\n",res);
            printf("#### sizeof info:%d\n",sizeof(info));
            printf("#### info.name:%s\n",info.name);
            printf("#### info.password:%s\n",info.password);
            printf("#### info.myfifo:%s\n",info.myfifo);
            
            int flag = 0; 
            for(int i=0;i<cookiesNum;i++)//匹配用户信息
            {
                if(!strcmp(cookies[i].name,info.name))
                {
                    if(!strcmp(cookies[i].password,info.password))
                    {
                        flag = 1;
                        printf("#### Find the user!\n");
                    }
                    break;
                }
            }
            if(flag)
            {
                //printf("flag:%d\n",flag);
                //printf("cookies[i].password:%s\n",cookies[i].password);
                //printf("info.password:%s\n",info.password);
                sprintf(buffer,"1");
                printf("login successes!\n");
            }
            else
            {
                sprintf(buffer,"0");
                printf("login failed!\n");
            }
            int fdl = open(info.myfifo, O_WRONLY);
            if(fdl == -1)
                printf("#### open info.myfifo failed!!!!\n");
            else
                printf("#### open info.myfifo successed!!\n");
            if(write(fdl, buffer, strlen(buffer)+1)==-1)
            {
                printf("Writing to fdl fails!!!errno:%d\n",errno);
                //perror();
            }
            else
            {
                printf("Writing back successes! buffer:%s\n",buffer);
            }
            close(fdl);
            printf("\n");
        }

        res = read(fifo_fd3, &info, sizeof(CLIENTINFO));    //读聊天信息
        if (res != -1){
            printf("3.Client'chat arrived!!res:%d\n",res);
            printf("#### sizeof info:%d\n",sizeof(info));
            printf("#### info.name:%s\n",info.name);
            printf("#### info.password:%s\n",info.password);
            printf("#### info.myfifo:%s\n",info.myfifo);
            printf("#### info.touser:%s\n",info.touser);
            printf("#### info.context:%s\n",info.context);
            int flag = -1;
            printf("#### check the touser\n");
            for(int i=0;i<cookiesNum;i++)
            {
                if(!strcmp(cookies[i].name,info.touser))
                {
                    flag = i;
                    break;
                }
                printf("#### user: %s is not user: %s\n",cookies[i].name,info.touser);
            }
            int fdl;
            if(flag != -1)
            {
                printf("#### Find the user!%s to %s",info.name,cookies[flag].name);
                fdl = open(cookies[flag].myfifo, O_WRONLY);
                sprintf(buffer,"%s:%s\n",info.name,info.context);

            }
            else//没找到聊天对象原路返回
            {
                printf("#### Do not find the user!%s",info.touser);
                sprintf(buffer,"error:the user do not exite");
                fdl = open(info.myfifo, O_WRONLY);
            }
            write(fdl, buffer, strlen(buffer)+1);
            close(fdl);
            printf("\n");
        }
        
       // close(fifo_fd1);close(fifo_fd2);close(fifo_fd3);
    }
    exit(0);
}
