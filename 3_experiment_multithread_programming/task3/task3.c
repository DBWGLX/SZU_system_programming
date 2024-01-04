#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NLOOP 5000

int counter;/*incremented by threads*/

void *increase(void *vptr);

//
pthread_mutex_t work_mutex;

int main(int argc, char **argv)
{
    //
    if(pthread_mutex_init(&work_mutex,NULL)!=0){perror("Mutex init failed");exit(1);}

    pthread_t threadIdA,threadIdB;

    pthread_create(&threadIdA,NULL,increase,NULL);
    pthread_create(&threadIdB,NULL,&increase,NULL);
    
    /*wait for both threads to terminate*/
    pthread_join(threadIdA,NULL);
    printf("#### threadIdA join!\n");
    pthread_join(threadIdB,NULL);
    printf("#### threadIdB join!\n");
    
    printf("#### Main completed\n");
    return 0;
}

void *increase(void *vptr)
{
    int i,val;

    for(i = 0;i < NLOOP; i++)
    {
        if(pthread_mutex_lock(&work_mutex)!=0){perror("Lock failed");exit(1);}

        val = counter;
        printf("%x: %d\n",(unsigned int)pthread_self(),val+1);
        counter = val + 1;
    
        if(pthread_mutex_unlock(&work_mutex)!=0){perror("unlock failed");exit(1);}
    }

    printf("#### pthread:%x has ended\n",(unsigned int)pthread_self());
    return NULL;
}
