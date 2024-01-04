#include <stdio.h>
#include <pthread.h>

void* func(void* vptr)
{
    printf("%x",pthread_self());
    return NULL;
}

int main()
{
    pthread_t t1,t2,t3,t4;
    for(int i=0;i<5;i++)
    {
        printf("p1");
        pthread_create(&t1,NULL,func,NULL);
        pthread_join(t1,NULL);
        printf("p2");
        pthread_create(&t2,NULL,func,NULL);
        pthread_join(t1,NULL);
        printf("p3");
        pthread_create(&t3,NULL,func,NULL);
        pthread_join(t1,NULL);
        printf("p4");
        pthread_create(&t4,NULL,func,NULL);  
        pthread_join(t1,NULL);
        
        printf("\n");
    }

   printf("\n");
    return 0;
}
