#ifndef TCNVMALLOC_H_
#define TCNVMALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdint.h>
#include "list.h"
#include "dlist.h"
#include <sys/types.h>

#define THREAD_LOCAL __attribute__ ((tls_model ("initial-exec"))) __thread
#define likely(x)           __builtin_expect(!!(x),1)
#define unlikely(x)         __builtin_expect(!!(x),0)
#define CACHE_LINE_SIZE     64
#define CACHE_ALIGN __attribute__ ((aligned (CACHE_LINE_SIZE)))
#define DEFAULT_BLOCK_CLASS (48)
#define LARGE_CLASS         (100)
#define DUMMY_CLASS         (101)
#define LARGE_OWNER         (0x5AA5)

/* configuration */
#define PAGE_SIZE 4096
#define CHUNK_DATA_SIZE (16*PAGE_SIZE)
#define CHUNK_SIZE (CHUNK_DATA_SIZE + sizeof(chunkh_t))
#define LHEAP_SIZE (sizeof(lheap_t))
#define RAW_POOL_START      ((void*)((0x600000000000/CHUNK_SIZE+1)*CHUNK_SIZE))
#define ALLOC_UNIT  (1024*1024*1024)
#define LHEAP_UNIT (128*LHEAP_SIZE)
#define WEAR_LIMIT  (10)

#define ROUNDUP(x,n)    ((x+n-1)/n * n);

typedef struct gpool_s gpool_t;
typedef struct gpool_s lheap_pool;
typedef struct lheap_s lheap_t;
typedef struct chunkh_s chunkh_t;

typedef enum {
    UNINIT,
    INITED
} thread_state_t;

typedef enum {
    FORG,
    BACK,
    FULL,
} chunk_state_t;

/* 内存块（chunk）的结构定义 */
struct chunkh_s {
    /* chunk所有者 */
    lheap_t *owner; 
    /* chunk状态 */
    chunk_state_t state;
    uint32_t size_cls;
    /* data block大小 */
    uint32_t blk_size;
    /* data block总数 */
    uint32_t blk_cnt;

    uint32_t free_mem_cnt;
    uint32_t free_tot_cnt;
    void *free_mem;

    /* FIFO free double-linked queue*/
    dlist_t dlist_head, dlist_tail;
    
    /* used to chain chunk in background list*/
    list_head list; 

    /* spare_count per allocation */
    int spare_count;

};

struct gpool_s {
    pthread_mutex_t lock;
    void *pool_start;
    void *pool_end;
    void *free_start;
    /* （已归还）空闲chunk链表 */
    list_head free_list;
};


struct lheap_s {
    chunkh_t *foreground[DEFAULT_BLOCK_CLASS];
    list_head background[DEFAULT_BLOCK_CLASS];

    /* lock for remote free, not affect free and malloc inside chunk */
    pthread_mutex_t lock;

    /* returned by local thread */
    list_head free_list;
    
    /* used to chain chunk in background list*/
    list_head list; 

    /* used to chain chunk freed by other thread */
    list_head remote_free;

    chunkh_t dummy_chunk;
};

/***********************************************************
 * TRmalloc的内存分配接口
 ***********************************************************/
void *tr_malloc(size_t size);
//void *tr_realloc(void *ptr, size_t size);

/***********************************************************
 * TRmalloc的内存释放接口
 ***********************************************************/
void tr_free(void *ptr);

/***********************************************************
 * TRmalloc的calloc接口
 * 功能：在内存的动态存储区中分配num个长度为size的连续空间
 * 函数返回一个指向分配起始地址的指针
 * 如果分配不成功，返回NULL。
 ***********************************************************/
inline static void *tr_calloc(size_t num, size_t size);

/***********************************************************
 * TRmalloc的realloc接口
 * 功能：尝试重新调整之前调用 malloc 或 calloc 所分配的 ptr 所指向的内存块的大小
 * 参数：
 * ptr -- 指针指向一个要重新分配内存的内存块
 *     -- 如果为空指针，则会分配一个新的内存块，且函数返回一个指向它的指针。
 * size -- 内存块的新的大小，以字节为单位
 *      -- 如果大小为 0，且 ptr 指向一个已存在的内存块，
 *      -- 则 ptr 所指向的内存块会被释放，并返回一个空指针。
 ***********************************************************/
inline static void *tr_realloc(void *ptr, size_t size);


/***********************************************************
 * 线程初始化函数
 * 获取一个local heap变量
 * 线程状态初始化
 ***********************************************************/
inline static void thread_init();

/***********************************************************
 * 检查线程是否第一次malloc
 * 首次malloc需要分配一个local heap结构体
 ***********************************************************/
inline static void check_init();

/***********************************************************
 * 全局堆初始化
 * 为进程申请一块空间
 * 同时初始化本地堆结构池
 ***********************************************************/
inline static void global_init();

/***********************************************************
 * 线程结束
 * 将local heap归还到结构池
 ***********************************************************/
inline static void thread_exit();   

/***********************************************************
 * global_init的子函数
 * 全局堆初始化
 * 同时初始化本地堆结构池
 * 初始化大小类映射全局变量
 ***********************************************************/
inline static void pool_init();

/***********************************************************
 * 初始化全局变量：
 * 建立大小和大小类的一一映射
 ***********************************************************/
inline static void maps_init();

/***********************************************************
 * tr_malloc的子函数
 * 分配小内存：size<=64KB
 ***********************************************************/
inline static void *small_malloc(int size_cls);

/***********************************************************
 * tr_malloc的子函数
 * 分配大内存：size>64KB
 ***********************************************************/
inline static void *large_malloc(size_t size);

/* global pool operation */
/***********************************************************
 * 全局堆扩张函数
 ***********************************************************/
inline static void gpool_grow();

/***********************************************************
 * 从全局堆中获取一个可用的chunk
 ***********************************************************/
inline static chunkh_t *gpool_acquire_chunk();

/* lheap pool operation */
/***********************************************************
 * 本地堆结构池扩张函数
 * 在本地堆结构体使用完后进行扩张
 ***********************************************************/
inline static void heap_pool_grow();   

/***********************************************************
 * 从本地堆结构池中获取一个可用的local heap结构体
 ***********************************************************/
inline static lheap_t *acquire_lheap();

/***********************************************************
 * 调用mmap申请一块空间
 ***********************************************************/
inline static void *page_alloc(void *pos, size_t size);

/***********************************************************
 * 调用munmap将空间归还给操作系统
 ***********************************************************/
inline static void page_free(void *pos, size_t size);

/***********************************************************
 * 从chunk中获取一个可用的data block 
 ***********************************************************/
inline static void *chunk_alloc_obj(chunkh_t *ch);

/***********************************************************
 * chunk初始化：
 ***********************************************************/
inline static void chunk_init(chunkh_t *ch, int size_cls);

/***********************************************************
 * 获取chunk的首地址
 * 输入:某个block的地址
 * 输出:该block所属的chunk地址
 * 用于获取某个block所属的chunk地址
 ***********************************************************/
inline static chunkh_t *chunk_extract_header(void *ptr);

/***********************************************************
 * tr_malloc的子函数
 * 输入:local heap地址
 * 用于从remote free list中回收blocks到本地堆的chunk中
 ***********************************************************/
inline static void remote_block_collect(lheap_t *lh);
/***********************************************************
 * tr_free的子函数
 * 输入:要释放的block地址及该block所属的chunk地址
 * 释放小内存：size<=64KB
 ***********************************************************/
inline static void chunk_free_small(chunkh_t *ch, void *ptr);
/***********************************************************
 * tr_free的子函数
 * 输入:要释放的block所属的chunk地址
 * 释放大内存：size>64KB
 ***********************************************************/
inline static void chunk_free_large(chunkh_t *ch);
/***********************************************************
 * tr_malloc的子函数
 * 输入:local heap指针以及要malloc的大小(该大小必须在大小类中)
 * 用于将本地堆中foreground chunk回收到background chunk list中
 * 同时从background chunk list中选出非满chunk换入foreground中
 * 如果background chunk list中没有非满chunk,则向global heap申请
 ***********************************************************/
inline static void lheap_replace_foreground(lheap_t *lh, int size_cls);
#ifndef NTMR
/***********************************************************
 * TR_Read接口
 * 输入:要读取的内存地址
 * 返回:经过三模冗余修复后的内存地址
 * 用于TR malloc用于纠错的接口,该函数会识别并修复ptr所指向的
 * block中被单bit翻转的数据,最后将修复后的地址返回
 ***********************************************************/
void * TR_Read(void* ptr);
/***********************************************************
 * TR_Write接口
 * 输入:要写入的内存地址,要写入的数据地址,写入数据大小
 * 返回:是否写入成功
 * 用于实现三模冗余的写入部分,每次写入三份数据到目标地址
 ***********************************************************/
intptr_t TR_Write(void* ptr, void* source, size_t size);


#endif
#ifdef __cplusplus
}
#endif

#endif
