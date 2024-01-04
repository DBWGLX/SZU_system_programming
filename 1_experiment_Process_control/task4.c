#include<iostream>
using namespace std;
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>

int powerOfTwo(int d)
{
    int ret = 1;
    for(int i=1;i<=d;i++)
    {   
        ret*=2;
    }
    return ret;
}

int main(int argc,char *argv[])
{
    
    if(argc != 2)
    {
        fprintf(stderr,"Usage : %s processes \n",argv[0]);return 1;
    }
    int* status;
    int d,n;
    d = atoi(argv[1]);
    n = powerOfTwo(d) - 2;
    int depth = d;
    pid_t pid1,pid2;
    int numb = 1;
    for(int i=0;i<depth ;i++){
        usleep(1500);
        printf("I am process no %5d  with PID %5d and PPID %d\n",numb , getpid(),getppid ());
        switch (pid1=fork()){
        case 0://子进程1
            numb=2*numb; break ;
        case -1:
            perror("fork"); exit(1);
        default://只有父进程
            pid2 = fork();
            usleep(500);
            switch (pid2){
                case 0://子进程2
                    numb=2*numb +1; break ;
                case -1:
                    perror("fork"); exit(1);
                default :
                    wait(status);
                    exit(0);
            }
        }
    }
    return 0;
}
