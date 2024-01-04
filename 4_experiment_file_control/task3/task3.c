#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>

char* str[] = {"","1 ","22 ","333 ","4444 "};
int fd[5];
int aimfd;

void* func(void* v)
{
    int i = *(int*)v;
    if(write(aimfd,str[i],strlen(str[i])) == -1){perror("Failed to write to file");exit(-2);}
}

int main()
{
    fd[1] = open("A",O_RDWR | O_APPEND);if(fd[1] == -1){perror("Failed to open A");exit(-1);};
    fd[2] = open("B",O_RDWR | O_APPEND);if(fd[2] == -1){perror("Failed to open B");exit(-1);};
    fd[3] = open("C",O_RDWR | O_APPEND);if(fd[3] == -1){perror("Failed to open C");exit(-1);};
    fd[4] = open("D",O_RDWR | O_APPEND);if(fd[4] == -1){perror("Failed to open D");exit(-1);};
    if(write(fd[2],str[2],strlen(str[2])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    if(write(fd[2],str[3],strlen(str[3])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    if(write(fd[2],str[4],strlen(str[4])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    if(write(fd[3],str[3],strlen(str[3])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    if(write(fd[3],str[4],strlen(str[4])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    if(write(fd[4],str[4],strlen(str[4])) ==-1){perror("Failed to write to file firstly");exit(-3);}
    int n1 = 1,n2 = 2,n3 = 3,n4 = 4;
    int tmp = 50;
    while(tmp--)
    {
        pthread_t thread1,thread2,thread3,thread4;
        for(int i=1;i<=4;i++)
        {
            aimfd = fd[i];
            if(pthread_create(&thread1,NULL,func,(void*)&n1) != 0){perror("Failed to create thread");exit(-4);}
            if(pthread_join(thread1,NULL) != 0){perror("Failed to join thread");exit(-5);}
            
            if(pthread_create(&thread2,NULL,func,(void*)&n2) != 0){perror("Failed to create thread");exit(-4);}
            if(pthread_join(thread2,NULL) != 0){perror("Failed to join thread");exit(-5);}
            
            if(pthread_create(&thread3,NULL,func,(void*)&n3) != 0){perror("Failed to create thread");exit(-4);}
            if(pthread_join(thread3,NULL) != 0){perror("Failed to join thread");exit(-5);}
            
            if(pthread_create(&thread4,NULL,func,(void*)&n4) != 0){perror("Failed to create thread");exit(-4);}
            if(pthread_join(thread4,NULL) != 0){perror("Failed to join thread");exit(-5);}
        }
    }
    return 0;
}


