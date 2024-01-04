#include "info.h"

//char mypipename[BUFF_SZ];

//void handler(int sig) { /* remove pipe if signaled */
//        unlink(mypipename);
//            exit(1);
//}


int main(int argc, char* argv[]) {
        int res;
        int fifo_fd, my_fifo;
        int fd;
        CLIENTINFO info;
        char buffer[BUFF_SZ];
        /*handle some signals */
        //signal(SIGKILL, handler);
        //signal(SIGINT, handler);
        //signal(SIGTERM, handler);
        /*check for proper command line */

        //输入参数数目不符合
        if (argc != 4) {
            printf("#### If you want to register,input : 0 name password!\n");
            printf("#### If you want to login,input : 1 name password!\n");
            printf("#### If you want to chat,after login please!\n");
            exit(1);
        }



        //读入参数
        int op = atoi(argv[1]);
        strcpy(info.name,argv[2]);
        strcpy(info.password,argv[3]);
        sprintf(info.myfifo,"/home/wangyaxian2022150054/5_final_assignment/%s",info.name);

     //   printf("#### 1.Check info:\n");
     //   printf("####       info.name:%s\n",info.name);
     //   printf("####       info.password:%s\n",info.password);
     //   printf("####       info.myfifo:%s\n",info.myfifo);

        //错误op
        if(op<0||op>1)
        {
            printf("#### Wrong opration!\n");
            exit(1);
        }
        else if(op == 0)//////////////////////////////////////////////  打开注册管道
        {
            /*check if server fifo exists */
            if (access(FIFO_NAME1, F_OK) == -1)                     {printf("Could not open FIFO %s\n", FIFO_NAME1);exit(EXIT_FAILURE);}
     //       printf("#### 0 accessed FIFO\n");
            /*open server fifo for write */
            fifo_fd = open(FIFO_NAME1, O_WRONLY);                   if (fifo_fd == -1) {printf("Could not open %s for write access\n",FIFO_NAME1);exit(EXIT_FAILURE);}
        }
        else if(op == 1)//////////////////////////////////////////////  打开登录管道
        {
            /*check if server fifo exists */
            if (access(FIFO_NAME2, F_OK) == -1)                     {printf("Could not open FIFO %s\n", FIFO_NAME2); exit(EXIT_FAILURE);}
     //       printf("#### 1 accessed FIFO\n");
            /*open server fifo for write */
            fifo_fd = open(FIFO_NAME2, O_WRONLY);                   if (fifo_fd == -1) {printf("Could not open %s for write access\n",FIFO_NAME2);exit(EXIT_FAILURE);}                   
        }
     //  printf("#### 2.have opened the FIFO_%d\n",op+1);



        /*create my own FIFO */
        //专属管道用来接受消息 
        if(access(info.myfifo,F_OK) == -1)
        {
            printf("#### 3.will mkfifo\n");
            res = mkfifo(info.myfifo, 0777);                             if (res != 0) {printf("FIFO %s was not created\n", info.myfifo);     exit(EXIT_FAILURE);}
        }
        /*open my own FIFO for reading */
   //     printf("#### 4.have made info.myfifo:%s\n",info.myfifo);

        /*write client info to server fifo*/
        write(fifo_fd, &info, sizeof(CLIENTINFO));
   //     printf("#### 5.have already sent to FIFO_%d\n",op+1);
   //     printf("####   sizeof CLIENTINFO:%d\n",sizeof(CLIENTINFO));
   //     printf("####   sizeof info:%d\n",sizeof(info));
   //     printf("####   info.name:%s\n",info.name);
   //     printf("####   info.password:%s\n",info.password);
   //     printf("####   info.myfifo:%s\n",info.myfifo);
        close(fifo_fd);



        my_fifo = open(info.myfifo, O_RDONLY/*|O_NONBLOCK*/);                       if (my_fifo == -1) {printf("Could not open %s for read only access\n",info.myfifo);  exit(EXIT_FAILURE);}
     //   printf("#### 6.have opened info.myfifo\n");





        //服务器向客户端返回注册或登录结果信息
        res = read(my_fifo, buffer, BUFF_SZ);
        if (res > 0) {
            printf("#### Received from server: %s\n", buffer);
        }



        //子进程一直读，
        pid_t pid1 = fork();
        if(pid1 == 0)
        {
            /*get result from server */
            printf("I am child%d,I am reading!\n",getpid());
            memset (buffer, '\0',BUFF_SZ);
            
            while(1)
            {
                fd_set readFds;
                FD_ZERO(&readFds);
                FD_SET(my_fifo, &readFds);
                int readyFdCount = select(my_fifo+1, &readFds, nullptr, nullptr, nullptr);
                if(FD_ISSET(my_fifo,&readFds))
                {
                    res = read(my_fifo, buffer, BUFF_SZ);
                    if (res > 0) {
                        printf("#### Received from server:\n%s\n", buffer);
                    }
                }
            }
        }
        else
        {
            printf("child %d is reading\n",pid1);
            if(op==0)
            {
                if(buffer[0] == '1')
                    printf("#### Register completes!\n");
                else
                    printf("#### Register fails,the name has been used!\n");
            }
            else if(op==1)
            {
                if(buffer[0] == '0')
                    printf("#### Login fails! The password you entered is wrong or the user does not exist!\n");
                else if(buffer[0] == '2')
                    printf("#### Login fails! There are too many people online!\n");
                else if(buffer[0] == '3')
                    printf("#### Login fails! The user is online now!\n"); 
                else//1
                {
                    printf("#### Login successes!\n");
                    printf("#### Now let's chat\n");
                    printf("#### Usage : name context\n");
                    printf("#### If you want to talk to more than one person,use this format:name,name,name context\n");
                    printf("#### If you want log out,input 0\n");
                    /*check if server fifo exists */
                    if (access(FIFO_NAME3, F_OK) == -1) {
                        printf("Could not open FIFO %s\n", FIFO_NAME3);
                        exit(EXIT_FAILURE);
                    }
                    /*open server fifo for write */
                    fifo_fd = open(FIFO_NAME3, O_WRONLY);
                    if (fifo_fd == -1) {
                        printf("Could not open %s for write access\n",
                                FIFO_NAME3);  exit(EXIT_FAILURE);
                    }
                   
                    //一直聊天吧
                    while(1)
                    {
                        scanf("%s",&info.touser);
                        //退出
                        if(info.touser[0] == '0')
                        {
                            fifo_fd = open(FIFO_NAME4, O_WRONLY);
                            if (fifo_fd == -1) {
                                printf("Could not open %s for write access\n",
                                FIFO_NAME4);  exit(EXIT_FAILURE);
                            }
                            write(fifo_fd, &info,sizeof(CLIENTINFO));
                            close(fifo_fd);
                            sleep(1);
                            break;
                        }
                        scanf("%s",&info.context);
                        /*open server fifo for write */
                        fifo_fd = open(FIFO_NAME3, O_WRONLY);
                        if (fifo_fd == -1) {
                            printf("Could not open %s for write access\n",
                                    FIFO_NAME3);
                            exit(EXIT_FAILURE);
                        }
                        /*write client info to server fifo*/
                        write(fifo_fd, &info, sizeof(CLIENTINFO));
                        close(fifo_fd);
                    }
                }
            }

            kill(pid1,9);
            printf("kill pid:%d\n",pid1);
            printf ( "Client %d is terminating\n" , getpid( ) ) ;
        }
        /*delete fifo from system */
        close(my_fifo) ;
        //(void) unlink (info.myfifo) ;//离线接受消息，就不用删除了
        exit(0 ) ;
}

