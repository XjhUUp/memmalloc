#include "memmalloc.h"
#include <assert.h>

#define THREAD_NUM 10

//测试抗单粒子翻转
void tmr_test()
{   
    printf("------TMR Test------\n");
    u_int32_t *test_num = (u_int32_t *)mem_malloc(sizeof(u_int32_t)*2000);
        int num = 12345;
        data_block_init(test_num);
        MEM_Write(test_num, &num, sizeof(u_int32_t));
        printf("Initial data: %d\n", *(u_int32_t *)MEM_Read(test_num));
        u_int32_t mask = 0xffffffef;
        // 模拟发生bit反转
        *test_num = *test_num & mask;
        printf("after ont bit corrupted: %d\n", *test_num);
        MEM_Read(test_num);
        printf("read data: %d\n", *test_num);
        int after_corrupted_num = *test_num;
        printf("test done!\n");
}

//单线程下验证扛单粒子翻转，验证后随机释放
void single_thread_tmr_test()
{
    printf("------single thread tmr test------\n");
    for (int i = 0; i <= 100000; i++)
    {
        u_int32_t *test_num = (u_int32_t *)mem_malloc(sizeof(u_int32_t));
        int num = 12345;
        data_block_init(test_num);
        MEM_Write(test_num, &num, sizeof(u_int32_t));
        u_int32_t mask = 0xffffffef;
        // 模拟发生bit反转
        *test_num = *test_num & mask;
        MEM_Read(test_num);
        int after_corrupted_num = *test_num;
        assert(after_corrupted_num==num);
        if(rand()%2==0){
            mem_free(test_num);
        }
    }
    printf("test done!\n");
}

void twork(void * arg){
    for (int i = 0; i <= 100000; i++)
    {
        u_int32_t *test_num = (u_int32_t *)mem_malloc(sizeof(u_int32_t));
        int num = 12345;
        data_block_init(test_num);
        MEM_Write(test_num, &num, sizeof(u_int32_t));
        u_int32_t mask = 0xffffffef;
        // 模拟发生bit反转
        *test_num = *test_num & mask;
        MEM_Read(test_num);
        int after_corrupted_num = *test_num;
        assert(after_corrupted_num==num);
        if(rand()%2==0){
            mem_free(test_num);
        }
    }

}

//单线程下验证扛单粒子翻转，验证后随机释放
void multi_thread_tmr_test(){
    printf("------multi thread tmr test------\n");
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

int main()
{
    tmr_test();
    //single_thread_tmr_test();
    //multi_thread_tmr_test();
}