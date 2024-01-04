#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct
{
    int value;
    char string[128];
}thread_parm_t;

void *threadfunc(void *parm)
{
    thread_parm_t *p = (thread_parm_t *)parm;
    printf("%s, parm = %d\n",p->string,p->value);
    free(p);
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t thread1,thread2,thread3,thread4,thread5;
    int rc1 = 0,rc2 = 0;
    thread_parm_t *parm = NULL;
    printf("Creat a thread attributes object\n");
    parm = malloc(sizeof(thread_parm_t));
    parm -> value = 1;
    strcpy(parm->string, "Inside the first thread");
    rc1 = pthread_create(&thread1,NULL,threadfunc,(void *)parm);
    
    parm = malloc(sizeof(thread_parm_t));
    parm->value = 2;
    strcpy(parm->string,"Inside the second thread");
    pthread_join(thread1,NULL);//
    rc2 = pthread_create(&thread2,NULL,threadfunc,(void *)parm);
    
    parm = malloc(sizeof(thread_parm_t));
    parm->value = 3;
    strcpy(parm->string,"Inside the third thread");
    pthread_join(thread2,NULL);//
    rc2 = pthread_create(&thread3,NULL,threadfunc,(void *)parm);
    
    parm = malloc(sizeof(thread_parm_t));
    parm->value = 4;
    strcpy(parm->string,"Inside the fourth thread");
    pthread_join(thread3,NULL);//
    rc2 = pthread_create(&thread4,NULL,threadfunc,(void *)parm);
    
    parm = malloc(sizeof(thread_parm_t));
    parm->value = 5;
    strcpy(parm->string,"Inside the fifth thread");
    pthread_join(thread4,NULL);//
    rc2 = pthread_create(&thread5,NULL,threadfunc,(void *)parm);
    
    pthread_join(thread5,NULL);//
    printf("Main completed\n");
    
    //printf("**** learn about thread1 : %d , thread2 : %d\n",thread1,thread2);
    //printf("**** learn about pthread_self() : %d\n",pthread_self());
    //printf("**** learn about " "1" " 2 " "\"\"\n");

    //printf("#### pthread_join()\n");
    //pthread_join(thread1,NULL);
    //pthread_join(thread2,NULL);
    return 0;
}
