#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#ifdef MEM_MALLOC
    #include "memmalloc.h"
#elif TR_MALLOC
    #include "TRmalloc.h"
#endif

#define BLKSIZE 256

int g_total_times = 8000;
int g_tdnum = 16;

void *twork(void *arg) {
    char *m;
    size_t i;
    for (i=0; i<g_total_times; i++) {
#ifdef MEM_MALLOC
        m = (char *)mem_malloc(BLKSIZE);
#elif TR_MALLOC
        m = (char *)tr_malloc(BLKSIZE);
#else
        m = (char *)malloc(BLKSIZE);
#endif

        if (rand() % 2 == 0) {
#ifdef MEM_MALLOC
        mem_free(m);
#elif TR_MALLOC
        tr_free(m);
#else
        free(m);
#endif
        }
    }

    return NULL;
}


int main(int argc, char *argv[]) {
    char *m;
    pthread_t tid[1000];
    int i, rc;
    srand(time(NULL));

    if (argc < 3) {
        fprintf(stderr, "usage: ./a.out <total_time> <threan_num>\n");
        exit(-1);
    }
    g_total_times = atoi(argv[1]);
    g_tdnum = atoi(argv[2]);

    for (i=0; i<g_tdnum; i++) {
        if (pthread_create(&tid[i], NULL, &twork, NULL) < 0) {
            printf("pthread_create err\n");
        }
    }

    for (i=0; i<g_tdnum; i++) {
        if (pthread_join(tid[i], NULL) < 0) {
            printf("pthread_join err\n");
        }
    }

    return 0;
}
