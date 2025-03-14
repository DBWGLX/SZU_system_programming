#include "info.h"


int res;
int fifo_fd/*服务器的一个管道的文件描述符 */,log_fd,chat_fd;//登录管道和聊天管道
int fd;
CLIENTINFO info;
char buffer[BUFF_SZ];
string str_log_fifo,str_chat_fifo;
string str_mydir;

bool Login(int argc, char* argv[]){
    printf("#### Welcome to use the chatting client!\n\n");
    //使用手册
    if (argc != 4) {
        printf("#### If you want to register,input : 0 name password!\n");
        printf("#### If you want to login,input : 1 name password!\n");
        exit(1);
    }

    //读入参数
    int op = atoi(argv[1]);
    strcpy(info.name,argv[2]);
    strcpy(info.password,argv[3]);
    sprintf(info.myfifo,"%s",info.name);

    //错误选项
    if(op<0||op>1){
        printf("#### Wrong opration!\n");
        exit(1);
    }
    else if(op == 0){//  打开服务器的注册管道
        /*check if server fifo exists */
        if (access(FIFO_NAME1, F_OK) == -1){
            printf("Could not open FIFO %s\n", FIFO_NAME1);
            exit(EXIT_FAILURE);
        }

        /*open server fifo for write */
        fifo_fd = open(FIFO_NAME1, O_WRONLY);
        if (fifo_fd == -1) {
            printf("Could not open %s for write access\n",FIFO_NAME1);
            exit(EXIT_FAILURE);
        }
    }
    else if(op == 1){//  打开登录管道
        /*check if server fifo exists */
        if (access(FIFO_NAME2, F_OK) == -1){
            printf("Could not open FIFO %s\n", FIFO_NAME2); 
            exit(EXIT_FAILURE);
        }
        /*open server fifo for write */
        fifo_fd = open(FIFO_NAME2, O_WRONLY);
        if (fifo_fd == -1) {
            printf("Could not open %s for write access\n",FIFO_NAME2);
            exit(EXIT_FAILURE);
        }                   
    }

    /*create my own FIFO */
    //用户的专属管道用来接受消息 
    str_mydir = string(info.myfifo);
    str_log_fifo = str_mydir + "/log";
    str_chat_fifo = str_mydir + "/chat";
    if(access(info.myfifo,F_OK) == -1){/*检查命名管道（FIFO）是否存在*/
        printf("#### 检测到当前用户在本地没有个人接收管道，正在创建...\n");
        
        res = mkdir(info.myfifo, 0777);    
        if(res != 0){
            printf("Failed to create the folder : %s\n", info.myfifo);     
            exit(EXIT_FAILURE);
        }

        res = mkfifo(str_log_fifo.c_str(),0777);
        if(res != 0){
            printf("Failed to create the FIFO : %s\n", str_log_fifo.c_str());     
            exit(EXIT_FAILURE);
        }

        
        res = mkfifo(str_chat_fifo.c_str(),0777);                    
        if (res != 0) {
            printf("Failed to create the FIFO : %s\n", str_chat_fifo.c_str());     
            exit(EXIT_FAILURE);
        }
        printf("#### 创建成功！\n");
    }
    //向服务器提交信息
    write(fifo_fd, &info, sizeof(CLIENTINFO));
    close(fifo_fd);
    //阻塞读，保证读到结果
    log_fd = open(str_log_fifo.c_str(), O_RDONLY);                       
    if (log_fd == -1) {
        printf("Could not open %s for read only access\n",str_log_fifo.c_str()); 
        perror("reason");
        exit(EXIT_FAILURE);
    }
    chat_fd = open(str_chat_fifo.c_str(), O_RDONLY|O_NONBLOCK);                       
    if (chat_fd == -1) {
        printf("Could not open %s for read only access\n",str_chat_fifo.c_str()); 
        exit(EXIT_FAILURE);
    }

    
    //服务器向客户端返回注册或登录结果信息
    res = read(log_fd, buffer, BUFF_SZ);
    if(op==0){
        if(buffer[0] == '1')
            printf("#### √ Register completes!\n");
        else
            printf("#### × Register fails,the name has been used!\n");
    }
    else if(op==1){
        if(buffer[0] == '0')
            printf("#### × Login fails! The password you entered is wrong or the user does not exist!\n");
        else if(buffer[0] == '2')
            printf("#### × Login fails! The server is full,try latter please.\n");
        else if(buffer[0] == '3')
            printf("#### × Login fails! The user is online now!\n"); 
        else if(buffer[0] == '1'){
            printf("#### √ Login successes! Now let's chat!\n");
            printf("#### To talk, use: [aim_name,aim_name...] [context]  such as: \"XiaoMing hello\"\n");
            printf("#### To log out, input 0\n");

            return true;
        }
        else{
            printf("The respond from server cannot be identified.Please contact the administrator\n");
        }
    }
    return false;
}


void Chat(){
    pid_t pid1 = fork();
    if(pid1 == 0){//子进程收
        /*get result from server */
        memset (buffer, '\0',BUFF_SZ);
        while(1){
            fd_set readFds;
            FD_ZERO(&readFds);
            FD_SET(chat_fd, &readFds);
            int readyFdCount = select(chat_fd+1, &readFds, nullptr, nullptr, nullptr);
            if(FD_ISSET(chat_fd,&readFds)){
                res = read(chat_fd, buffer, BUFF_SZ);
                if (res > 0) {
                    printf("【Received from server:】\n%s\n", buffer);
                }
            }
        }
    }
    else{//父进程发
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
                
        while(1){
            scanf("%s",&info.touser);
            //退出
            if(info.touser[0] == '0'){
                fifo_fd = open(FIFO_NAME4, O_WRONLY);
                if (fifo_fd == -1) {
                    printf("Could not open %s for write access\n",FIFO_NAME4);  
                    exit(EXIT_FAILURE);
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
                printf("Could not open %s for write access\n",FIFO_NAME3);
                exit(EXIT_FAILURE);
            }
            /*write client info to server fifo*/
            write(fifo_fd, &info, sizeof(CLIENTINFO));
            close(fifo_fd);
        }
        
        kill(pid1,9);//杀死子进程
    }
}

int main(int argc, char* argv[]) {

    if(Login(argc, argv)){
        Chat();
    }

    /*delete fifo from system */
    close(log_fd);
    close(chat_fd);
    return 0;
}

