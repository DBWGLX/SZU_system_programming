#include "info.h"

void handler(int sig){
//    unlink(FIFO_NAME);
//    exit(1);
}

CLIENTINFO cookies[150];
int cookiesNum = 0;
int online_once[50] = {0};

#define MAX_ONLINE_USERS 4
//
    int res;
    int i;
    int fifo_fd1,fifo_fd2,fifo_fd3,fifo_fd4,fdl;
    CLIENTINFO info;
    char buffer[150];
    int maxFd;
    int online_num = 0;

void* threadFunc1(void* arg)
{
    res = read(fifo_fd1, &info, sizeof(CLIENTINFO));    //1.读注册信息 info
    if (res != -1){
        printTime();
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
        printf("#### have fed back!\n");
        close(fdl);
    }
    //else{printf("!!!! read register info FAILED!\n");}

    fflush(NULL);
}
void* threadFunc2(void* arg)
{
    res = read(fifo_fd2, &info, sizeof(CLIENTINFO));    //2.读登录信息
    if (res != -1){
        printTime();
        printf("2.Client'login arrived!!res:%d\n",res);
        printf("#### sizeof info:%d\n",sizeof(info));
        printf("#### info.name:%s\n",info.name);
        printf("#### info.password:%s\n",info.password);
        printf("#### info.myfifo:%s\n",info.myfifo);

        int flag = 0;
        if(online_num < MAX_ONLINE_USERS)
        {
            for(int i=0;i<cookiesNum;i++)//匹配用户信息
            {
                if(!strcmp(cookies[i].name,info.name))
                {
                    if(!strcmp(cookies[i].password,info.password))
                    {
                        flag = 1;
                        printf("#### Find the user!\n");
                        if(online_once[i] == 1)
                        {
                            flag = 3;
                        }
                        else
                            online_once[i] = 1;
                    }
                    break;
                }
            }
            if(flag == 1)
            {
                //printf("flag:%d\n",flag);
                //printf("cookies[i].password:%s\n",cookies[i].password);
                //printf("info.password:%s\n",info.password);
                sprintf(buffer,"1");
                printf("login successes!\n");
                online_num++;
                printf("online_num:%d\n",online_num);
            }
            else if(flag == 3)
            {
                sprintf(buffer,"3");
                printf("login failed! the user is online now!\n");
            }
            else
            {
                sprintf(buffer,"0");
                printf("login failed! Wrong password!\n");
            }
        }
        else
        {
            sprintf(buffer,"2");
            printf("login failed! There are too many people online\n");
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
        
        //将在线用户名显示给用户
        if(flag == 1)
        {
            sprintf(buffer,"== %s: Now server has %d people online:\n",getTime().c_str(),online_num);
            int tmp = 1,offset = strlen(buffer);
            for(int i=0;i<cookiesNum;i++)//匹配用户信息
            {
                if(online_once[i] == 1)
                {
                    sprintf(buffer+offset,"    %d.%s\n",tmp++,cookies[i].name);
                    offset = strlen(buffer);
                }
            }
            //发送
            for(int i=0;i<cookiesNum;i++)
            {
                printf("#### online information is sent to %s\n",cookies[i].name);
                fdl = open(cookies[i].myfifo, O_WRONLY);
                if(fdl == -1)
                {
                    printf("!!!! fail to send to %s : open fifo failed!\n",cookies[i].name);
                    continue;
                }
                write(fdl, buffer, strlen(buffer)+1);
                close(fdl);
            }
        }
        
        
        printf("\n");
    }
    //else{printf("!!!! read login info FAILED!\n");}

    fflush(NULL);
}   
void* threadFunc3(void* arg)
{
    res = read(fifo_fd3, &info, sizeof(CLIENTINFO));    //读聊天信息
    if (res != -1){
        printTime();
        printf("3.Client'chat arrived!!res:%d\n",res);
        printf("#### sizeof info:%d\n",sizeof(info));
        printf("#### info.name:%s\n",info.name);
        printf("#### info.password:%s\n",info.password);
        printf("#### info.myfifo:%s\n",info.myfifo);
        printf("#### info.touser:%s\n",info.touser);
        printf("#### info.context:%s\n",info.context);
        int flag = -1;
        printf("#### check the touser\n");
        //处理单个对象或者多个对象，touser是“Jack,Amy,Lucy”形式
        const char delimiter[] = ",";
        char* token = strtok(info.touser,delimiter);
        while(token != NULL)
        {
            printf("#### touser:%s\n",token);
            for(int i=0;i<cookiesNum;i++)
            {
                if(!strcmp(cookies[i].name,token))
                {
                    flag = i;
                    break;
                }
                //printf("#### user: %s is not user: %s\n",cookies[i].name,token);
           }
            int fdl;
            if(flag != -1)
            {
                //printf("#### Find the user!%s to %s\n",info.name,cookies[flag].name);
                fdl = open(cookies[flag].myfifo, O_WRONLY);
                sprintf(buffer,"== %s %s:%s\n",getTime().c_str(),info.name,info.context);
     
            }
            else//没找到聊天对象原路返回
            {
                printf("#### Do not find the user!%s\n",info.touser);
                sprintf(buffer,"error:the user do not exite");
                fdl = open(info.myfifo, O_WRONLY);
            }
            write(fdl, buffer, strlen(buffer)+1);
            close(fdl);

            token = strtok(NULL,delimiter);
        }
        printf("\n");
    }
    //else{printf("!!!! read user's sending FAILED!\n");}

    fflush(NULL);
}
void* threadFunc4(void* arg)
{
    res = read(fifo_fd4, &info, sizeof(CLIENTINFO));    //读登出信息
    if (res != -1){
        printTime();
        printf("4.Client'logout arrived!!res:%d\n",res);
        printf("#### sizeof info:%d\n",sizeof(info));
        printf("#### info.name:%s\n",info.name);
        printf("#### info.password:%s\n",info.password);
        printf("#### info.myfifo:%s\n",info.myfifo);
        printf("#### info.touser:%s\n",info.touser);
        printf("#### info.context:%s\n",info.context);
        
        if(info.touser[0] == '0')
        {
            sprintf(buffer,"log out success!");
            online_num--;
            printf("online_num:%d\n",online_num);
            for(int i=0;i<cookiesNum;i++)//匹配用户信息
            {
                if(!strcmp(cookies[i].name,info.name))
                {
                    online_once[i] = 0;
                    break;
                }
            }

        }
        else
            sprintf(buffer,"log out fail!just only input '0' please!");
        fdl = open(info.myfifo, O_WRONLY);
        write(fdl, buffer, strlen(buffer)+1);
        close(fdl);
        printf("\n");
    
    
        //将在线用户名显示给用户
            sprintf(buffer,"== %s: Now server has %d people online:\n",getTime().c_str(),online_num);
            int tmp = 1,offset = strlen(buffer);
            for(int i=0;i<cookiesNum;i++)//匹配用户信息
            {
                if(online_once[i] == 1)
                {
                    sprintf(buffer+offset,"    %d.%s\n",tmp++,cookies[i].name);
                    offset = strlen(buffer);
                }
            }
            //发送
            for(int i=0;i<cookiesNum;i++)
            {
                printf("#### online information has been sent to %s\n",cookies[i].name);
                fdl = open(cookies[i].myfifo, O_WRONLY);
                if(fdl == -1)
                {
                    printf("!!!! fail to send to %s");
                    continue;
                }
                write(fdl, buffer, strlen(buffer)+1);
                close(fdl);
            }

    }
    //else{printf("!!!! read logout info FAILED!\n");}
    fflush(NULL);
}
int main(){

    pid_t pid;
    pid = fork();
    if (pid < 0) {  perror("Fork failed");fflush(NULL);exit(1);}

    // 父进程退出
    if (pid > 0) {  exit(0); }
    // 子进程继续执行

    // 创建新会话并成为会话首进程
    if (setsid() < 0) {
        perror("setsid error");
        exit(1);
    }

    // 改变工作目录
    if (chdir("/") < 0) {
        perror("chdir error");
        exit(1);
    }

    //建立日志文件
    int file = open(LOG_TXT,O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(file == -1)
    {
        printf("Failed to open file\n");
        fflush(NULL);
        exit(0);
    }

    freopen("/dev/null", "r", stdin);
    freopen(LOG_TXT, "w", stdout);
    freopen(LOG_TXT, "w", stderr);
  
    ///
        memset(cookies,0,sizeof cookies);
    //signal(SIGKILL, handler);
    //signal(SIGINT, handler);
    signal(SIGTERM, handler);
    signal(SIGCHLD, handler);

    /* create FIFO, if necessary */
    //建立管道
    if (access(FIFO_NAME1, F_OK) == -1) {
        res = mkfifo(FIFO_NAME1, 0777);
        if(res != 0) {
            printTime();
            printf("FIFO1 %s was not created\n", FIFO_NAME1);
            fflush(NULL);
            exit(EXIT_FAILURE);
        }
    }
    if (access(FIFO_NAME2, F_OK) == -1) {
        res = mkfifo(FIFO_NAME2, 0777);
        if(res != 0) {
            printTime();
            printf("FIFO2 %s was not created\n", FIFO_NAME2);
            fflush(NULL);
            exit(EXIT_FAILURE);
        }
    }
    if (access(FIFO_NAME3, F_OK) == -1) {
        res = mkfifo(FIFO_NAME3, 0777);
        if(res != 0) {
            printTime();
            printf("FIFO3 %s was not created\n", FIFO_NAME3);
            fflush(NULL);
            exit(EXIT_FAILURE);
        }
    }
    if (access(FIFO_NAME4, F_OK) == -1) {
        res = mkfifo(FIFO_NAME4, 0777);
        if(res != 0) {
            printTime();
            printf("FIFO4 %s was not created\n", FIFO_NAME4);
            fflush(NULL);
            exit(EXIT_FAILURE);
        }
    }/* create FIFO, if necessary */

    printTime();
    printf("\nServer is rarin' to go!\n");
    fflush(NULL);

    /* open FIFO for reading */
    //分别打开肆个管道
    fifo_fd1 = open(FIFO_NAME1, O_RDWR|O_NONBLOCK);
    if(fifo_fd1 == -1){
        printTime();
        printf("Could not open %s for read only access\n",
                FIFO_NAME1);
        fflush(NULL);
        exit(EXIT_FAILURE);
    }
    fifo_fd2 = open(FIFO_NAME2, O_RDWR|O_NONBLOCK);
    if(fifo_fd2 == -1){
        printTime();
        printf("Could not open %s for read only access\n",
                FIFO_NAME2);
        fflush(NULL);
        exit(EXIT_FAILURE);
    }
    fifo_fd3 = open(FIFO_NAME3, O_RDWR|O_NONBLOCK);
    if(fifo_fd3 == -1){
        printTime();
        printf("Could not open %s for read only access\n",
                FIFO_NAME3);
        fflush(NULL);
        exit(EXIT_FAILURE);
    }
    fifo_fd4 = open(FIFO_NAME4, O_RDWR|O_NONBLOCK);
    if(fifo_fd4 == -1){
        printTime();
        printf("Could not open %s for read only access\n",
                FIFO_NAME4);
        fflush(NULL);
        exit(EXIT_FAILURE);
    }

    maxFd = max( max( max(fifo_fd1,fifo_fd2) , fifo_fd3 ), fifo_fd4 ) + 1;

    while(1)
    {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(fifo_fd1, &readFds);
        FD_SET(fifo_fd2, &readFds);
        FD_SET(fifo_fd3, &readFds);
        FD_SET(fifo_fd4, &readFds);
        
        pthread_t thread;

        int readyFdCount = select(maxFd, &readFds, nullptr, nullptr, nullptr);
        if (readyFdCount == -1) {printTime();printf("select() 错误");break;}

        if(FD_ISSET(fifo_fd1,&readFds))
        {
            if (pthread_create(&thread, NULL, threadFunc1, NULL) != 0) {
                perror("Failed to create thread");
            }
            
            if (pthread_detach(thread) != 0) { perror("Failed to detach thread");}
       
        }

        if(FD_ISSET(fifo_fd2,&readFds))
        {
            if (pthread_create(&thread, NULL, threadFunc2, NULL) != 0) {
                perror("Failed to create thread");}
            
            if (pthread_detach(thread) != 0) { perror("Failed to detach thread");}

        }

        if(FD_ISSET(fifo_fd3,&readFds))
        {
            if (pthread_create(&thread, NULL, threadFunc3, NULL) != 0) {
                perror("Failed to create thread");}
            
            if (pthread_detach(thread) != 0) { perror("Failed to detach thread");}

        }

        if(FD_ISSET(fifo_fd4,&readFds))
        {
            if (pthread_create(&thread, NULL, threadFunc4, NULL) != 0) {
                perror("Failed to create thread");}
            
            if (pthread_detach(thread) != 0) { perror("Failed to detach thread");}

        }
       // close(fifo_fd1);close(fifo_fd2);close(fifo_fd3);

       // printTime();
       //printf("&&&& cheack the cookiesNum:%d\n",cookiesNum);
        fflush(NULL);
    }
    exit(0);
}

