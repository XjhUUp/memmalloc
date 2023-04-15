/*
本文件用于TRmalloc的正确性测试,包括TRmalloc
TRfree, TRrealloc, TRRead和TRWrite
*/
#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h> 
#include <pthread.h>
#include <assert.h>
#include "TRmalloc.h"
#define TEST_NUM 100
#define TR_MALLOC
#define TR_FREE
#define TR_REALLOC

// variable for getopt
extern char* optarg;
extern int optind;
extern int opterr;
extern int optopt;

void test1();
void test2();
void test3();
void test4();
void test5();
void test6();
void test7();

struct tests
{
    int total;
    int test_id[TEST_NUM];
};

void print_help(char* exe_name) {
  printf("Usage: %s\n [ -h ] [ -t testnumber ] [ -a ]\n", exe_name);
}

int main(int argc, char **argv) {
    int ch;  
    int test_num = 0;
    struct tests testcase;
    testcase.total = 0;
    opterr = 0;
    while ((ch = getopt(argc,argv,"aht:"))!=-1)
    {  
        switch(ch)  
        {  
            case 't':
                    testcase.test_id[testcase.total] = atoi(optarg);
                    testcase.total++;
                    break;  
            case 'h':  
                    print_help(argv[0]);
                    break; 
            case 'a':  
                    goto testall;
                    break;  
            default:  
                    print_help(argv[0]);
                    return 0;
        }  
    }
    for(int i = 0; i < testcase.total; i++){
        switch (testcase.test_id[i])
        {
        case 0:
            break;
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;
        case 3:
            test3();
            break;
        case 4:
            test4();
            break;
        case 5:
            test5();
            break;
        case 6:
            test6();
            break;
        case 7:
            test7();
            break;
        
        default:
            printf("No such test!\n");
            break;
        }
    }
    return 0;
testall:
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    return 0;
}

// test for a single malloc
void test1(){
    printf("------start test 1------\n");  
    #ifdef TR_MALLOC
    void *addr;
    addr = tr_malloc(100);
    printf("addr:%p\n", addr);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}

// test for a single malloc and free
void test2(){
    printf("------start test 2------\n");  
    #ifdef TR_FREE
    void *addr;
    addr = tr_malloc(100);
    printf("addr:%p\n", addr);
    tr_free(addr);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}

// test for several continuous malloc
void test3(){
    printf("------start test 3------\n");  
    #ifdef TR_MALLOC
    void *addr;
    addr = tr_malloc(0);
    printf("addr:%p\n", addr);
    addr = tr_malloc(1);
    printf("addr:%p\n", addr);
    addr = tr_malloc(10);
    printf("addr:%p\n", addr);
    addr = tr_malloc(100);
    printf("addr:%p\n", addr);
    addr = tr_malloc(1000);
    printf("addr:%p\n", addr);
    addr = tr_malloc(10000);
    printf("addr:%p\n", addr);
    addr = tr_malloc(100000);
    printf("addr:%p\n", addr);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}

// test for several continuous malloc and free
void test4(){
    printf("------start test 4------\n");  
    #ifdef TR_FREE
    void *addr;
    int64_t mount_addr;
    int64_t chunk_addr;
    addr = tr_malloc(100000);
    tr_free(addr);
    mount_addr = (int64_t)addr >> 20;
    addr = tr_malloc(10000);
    tr_free(addr);
    chunk_addr = (int64_t)addr >> 20;
    assert(chunk_addr != mount_addr);
    addr = tr_malloc(0);
    tr_free(addr);
    assert(chunk_addr == (int64_t)addr>>20);
    addr = tr_malloc(1);
    tr_free(addr);
    assert(chunk_addr == (int64_t)addr>>20);
    addr = tr_malloc(10);
    tr_free(addr);
    assert(chunk_addr == (int64_t)addr>>20);
    addr = tr_malloc(100);
    tr_free(addr);
    assert(chunk_addr == (int64_t)addr>>20);
    addr = tr_malloc(1000);
    tr_free(addr);
    assert(chunk_addr == (int64_t)addr>>20);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}

// malloc a total chunk
void test5(){
    printf("------start test 5------\n");  
    #ifdef TR_FREE
    void *addr;
    void *first_addr;
    void *second_addr;
    first_addr = tr_malloc(15);
    tr_free(first_addr);
    second_addr = tr_malloc(15);
    tr_free(second_addr);
    for(int i = 0; i < CHUNK_DATA_SIZE/(16*3) - 2; i++){
        addr = tr_malloc(15);
        tr_free(addr);
    }
    addr = tr_malloc(15);
    assert(addr == first_addr);
    addr = tr_malloc(15);
    assert(addr == second_addr);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}

// 1 bit correction
void test6(){
    printf("------start test 6------\n");  
    #ifdef TR_FREE
    uint32_t *test_num = (uint32_t*)tr_malloc(sizeof(uint32_t));
    int num = 12345;
    printf("before TR_Write: %d\n", *(uint32_t *)TR_Read(test_num));
    TR_Write(test_num, &num, sizeof(uint32_t));
    printf("after TR_Write: %d\n", *(uint32_t *)TR_Read(test_num));
    uint32_t mask = 0xffffffef;
    // 模拟发生bit反转
    *test_num = *test_num & mask;
    assert(*test_num == (12345 & 0xffffffef));
    printf("ont bit corrupted: %d\n", *test_num);
    printf("TR_read corrupted data: %d\n", *(uint32_t *)TR_Read(test_num));
    assert(*test_num == 12345);
    printf("after TR_Read: %d\n", *test_num);
    printf("test done!\n");
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}
pthread_mutex_t mutex;
// 用于模拟正常的读进程
void *rthread(void *num_ptr) {
    uint32_t* addr = num_ptr;
    int read_count = 0;
    printf("read thread started\n");
    int i = 0;

    while(i < 1000000){
        pthread_mutex_lock(&mutex);
        int num = *(uint32_t *)TR_Read(addr);
        assert(num == 12345);
        read_count++;
        pthread_mutex_unlock(&mutex);
        i++;
    }
    printf("total read %d\n",read_count);
    return NULL;
}
// 用于模拟单比特反转的进程
void *wthread(void *num_ptr) {
    uint32_t* addr = num_ptr;
    int corrupt_count = 0;
    printf("write thread started\n");
    int i = 0;
    while(i<1000000){
        if(*addr == 12345){
            pthread_mutex_lock(&mutex);
            *addr = *addr & 0xffffffef;
            corrupt_count++;
            pthread_mutex_unlock(&mutex);
        }
        i++;
    }
    printf("total corrupt %d\n",corrupt_count);

    return NULL;
}

void test7(){
    printf("------start test 7------\n");  
    #ifdef TR_FREE
    int num = 12345;
    uint32_t *addr = tr_malloc(sizeof(int));
    TR_Write(addr, &num, sizeof(uint32_t));
    pthread_mutex_init(&mutex, NULL);

    pthread_t tid[2];
    if (pthread_create(&tid[0], NULL, &wthread, (void*)addr) < 0) {
        printf("thread create error\n");
    }
    if (pthread_create(&tid[1], NULL, &rthread, (void*)addr) < 0) {
        printf("thread_create error\n");
    }
    
    if (pthread_join(tid[0], NULL) < 0) {
        printf("thread join error\n");
    }
    if (pthread_join(tid[1], NULL) < 0) {
        printf("thread join error\n");
    }
    #else
    printf("Function required hasn't been perpared!\n");
    #endif
}