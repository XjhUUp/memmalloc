#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#ifdef MEM_MALLOC
    #include "memmalloc.h"
#elif TR_MALLOC
    #include "TRmalloc.h"
#endif

int g_total_times = 8000;
int g_tdnum = 16;

void rwtest(void *addr) {
    int* num = (int*)addr;
    register int temp;
    int i;

    for (i=0; i<g_total_times; i++) {
#ifdef MEM_MALLOC
        temp = *(int *)MEM_Read(addr);
#elif  TR_MALLOC
        temp = *(int *)TR_Read(addr);
#else
        temp = *num;
#endif


#ifdef MEM_MALLOC
        MEM_Write(addr, &i, sizeof(int));
#elif  TR_MALLOC
        TR_Write(addr, &i, sizeof(int));
#else
        *num = i;
#endif
        
    }
}


int main(int argc, char *argv[]) {
    pthread_t tid[1000];
    int i;
    void *addr;
    srand(time(NULL));

    if (argc < 3) {
        fprintf(stderr, "usage: ./a.out <total_time>\n");
        exit(-1);
    }
    g_total_times = atoi(argv[1]);
    g_tdnum = atoi(argv[2]);
    
    struct timeval begin, end;
#ifdef MEM_MALLOC
    addr = (int*)mem_malloc(sizeof(int));
#elif TR_MALLOC
    addr = (int*)tr_malloc(sizeof(int));
#else
    addr = (int*)malloc(sizeof(int));
#endif
    gettimeofday(&begin, NULL);
    rwtest(addr);
    gettimeofday(&end, NULL);

    int time_in_us = (end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec);
    printf("%lf", time_in_us / (double)(g_tdnum * g_total_times));


    return 0;
}
