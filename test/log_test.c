#include "memmalloc.h"
#include "log.h"
#include <stdio.h>

#define RODOM_BLOCK 15000
#define THREAD_NUM 10

//单线程下获取日志信息，分配随机大小内存块，并随机释放
void single_thread_log_test(){
    printf("------single thread log test------\n");
    void *addr;
    for(int i=0;i<=100;i++){
        int j=rand()%RODOM_BLOCK;
        addr = mem_malloc(j);
        if(rand()%2==0){
            mem_free(addr);
        }
    }
    printf("test done!\n");
}

//多线程下获取日志信息，分配随机大小内存块，并随机释放
void twork(void *arg){
    void *addr;
    for(int i=0;i<=100;i++){
        int j=rand()%RODOM_BLOCK;
        addr = mem_malloc(j);
        if(rand()%2==0){
            mem_free(addr);
        }
    }
}

void multi_thread_log_test(){
    printf("------multi thread log test------\n");
    pthread_t tid[100];
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (pthread_create(&tid[i], NULL, &twork, NULL) < 0)
        {
            printf("pthread_create err\n");
        }
    }
    for (int i = 0; i < THREAD_NUM; i++)
    {
        if (pthread_join(tid[i], NULL) < 0)
        {
            printf("pthread_join err\n");
        }
    }

     printf("test done!\n");
}




int main(){
    // single_thread_log_test();
    // multi_thread_log_test();
   
}
