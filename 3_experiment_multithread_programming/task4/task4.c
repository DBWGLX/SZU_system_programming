#include <stdio.h>
#include <pthread.h>


void* routine(void *vptr)
{

    for(int i=0;i<2;i++)
    {
        printf("thread:%d\n",i);
    }
    return NULL;
}

char strs[10][10] = {"first","second","third","fourth","fifth"};

int main()
{
    pthread_t thread;


    for(int i=0;i<5;i++)
    {
        printf("#### %s cycle:\n",strs[i]);

        pthread_create(&thread,NULL,routine,NULL);

        pthread_join(thread,NULL);

        for(int j=0;j<4;j++)
        {
            printf("main:%d\n",j);
        }
    }

    return 0;
}
