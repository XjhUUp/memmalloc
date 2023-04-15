#include "TRmalloc.h"
#include "pthread.h"
#include<unistd.h>
#include<string.h>

/* Global metadata */
pthread_once_t init_once = PTHREAD_ONCE_INIT;
thread_state_t global_state = UNINIT;
pthread_key_t destructor;
gpool_t gpool;
lheap_pool heap_pool;   //local heap struct pool

/* Mappings */
CACHE_ALIGN int cls2size[128];
char sizemap[256];
char sizemap2[128];

/* threads */
THREAD_LOCAL thread_state_t thread_state = UNINIT;
THREAD_LOCAL lheap_t *local_heap = NULL;


static void pool_init() {
    if (pthread_mutex_init(&gpool.lock, NULL)<0 
        || pthread_mutex_init(&heap_pool.lock, NULL)<0) {
        fprintf(stderr, "fatal error: pthread_mutex_init failed\n");
        exit(-1);
    }
    /* 全局堆预先分配一块大内存 */
    void *ret = page_alloc(RAW_POOL_START, ALLOC_UNIT);
    if (ret < (int)0) {
        fprintf(stderr, "fatal error: page_alloc ailed\n");
        exit(-1);
    }

    /* 全局堆指针初始化 */
    gpool.pool_start = (void *)(((uint64_t)ret + CHUNK_SIZE - 1)/CHUNK_SIZE * CHUNK_SIZE);
    gpool.pool_end = ret + ALLOC_UNIT;
    gpool.free_start = gpool.pool_start;

    INIT_LIST_HEAD(&gpool.free_list);

    /* 预先分配若干本地堆结构体 */
    ret = page_alloc(NULL, LHEAP_UNIT);
    if (ret < (int)0) {
        fprintf(stderr, "fatal error: page_alloc ailed\n");
        exit(-1);
    }
    heap_pool.pool_start = (void *)(((uint64_t)ret + LHEAP_SIZE - 1)/LHEAP_SIZE * LHEAP_SIZE);
    heap_pool.pool_end = ret + LHEAP_UNIT;
    heap_pool.free_start = heap_pool.pool_start;
    INIT_LIST_HEAD(&heap_pool.free_list);
}

static void maps_init() {
    int size;
    int class;

    /* 大小类：<=256B，以8B为单位递增 */
    for (size = 16, class = 0; size <= 256; size += 8, class++) {
        cls2size[class] = size;
    }

    /*
     * 大小类：256B<size<=65536 
     * 递增公式：
     * a_n=1.5a_{n-1}
     * a_{n+1}=2a_{n-1}
     */
    for (size = 256; size < 65536; size <<= 1) {
        cls2size[class++] = size + (size >> 1);
        cls2size[class++] = size << 1;
    }

    int cur_class = 0;
    int cur_size = 0;

    /* init sizemap */
    /* 辅助全局sizemap，用来快速获得<=1024B空间的大小类 */
    for (cur_size = 8; cur_size <= 1024; cur_size += 8) {
        if (cur_size > cls2size[cur_class])
            cur_class++;
        sizemap[(cur_size - 1) >> 3] = cur_class;
    }
    
    /* init sizemap2 */
    /* 辅助全局sizemap2，用来快速获得1024B<size<=65536空间的大小类 */
    for (cur_size = 1024; cur_size <= 65536; cur_size += 512) {
        if (cur_size > cls2size[cur_class])
            cur_class++;
        sizemap2[(cur_size - 1) >> 9] = cur_class;
    }
}

static void thread_exit() {

    /* 线程退出时将local heap结构体归还到pool */
    list_add_tail(&local_heap->list, &heap_pool.free_list);
    //free(local_heap);
}

static void global_init() {
    pthread_key_create(&destructor, thread_exit);
    pool_init();
    maps_init();
    global_state = INITED;
}

static void check_init() {
    /* 检查线程是不是第一次malloc */
    if (unlikely(thread_state != INITED)) {
        if (unlikely(global_state != INITED)) {
            pthread_once(&init_once, global_init);
        }
        /* 线程如果是第一次malloc，则给它分配一个local heap结构体 */
        thread_init();
    }
}

static void thread_init() {
    /* make thread_exit executed when thread quit */
    pthread_setspecific(destructor, (void *)1);
    local_heap = acquire_lheap();
    thread_state = INITED;
}

static void *chunk_alloc_obj(chunkh_t *ch) {
    void *ret = NULL;

    /* 如果还有未分配过的数据块，则直接分配一个block */
    if (ch->free_mem_cnt > 0) {
        ret = ch->free_mem;
        ch->free_mem += ch->blk_size;
        ch->free_mem_cnt--;
    } 
    /* 如果已经没有未分配的block，则从空闲block列表中获取 */
    else{
        ret = (void *)ch->dlist_tail.prev;
        dlist_remove((dlist_t *)ret);
    }
    ch->free_tot_cnt--;

    return ret;
}

static void chunk_init(chunkh_t *ch, int size_cls) {

    /* chunk初始化，设置大小类，填写元数据*/
    ch->size_cls = size_cls;
    ch->blk_size = cls2size[size_cls];   

    /* 计算切割出多少个data block */ 
    ch->blk_cnt = (CHUNK_DATA_SIZE) / ch->blk_size;
    ch->free_tot_cnt = ch->free_mem_cnt = ch->blk_cnt;
    ch->free_mem = (void *)ch + sizeof(chunkh_t);
    
    /* 空闲数据块链表初始化 */
    dlist_init(&ch->dlist_head, &ch->dlist_tail);
    
    ch->spare_count = 0;
}

static chunkh_t *chunk_extract_header(void *ptr) {
    /* 返回内存块的首地址 */
    return (chunkh_t *)((uint64_t)ptr - (uint64_t)(ptr) % CHUNK_SIZE);
}

void lheap_replace_foreground(lheap_t *lh, int size_cls) {
    chunkh_t *ch;

    if (!list_empty(&(lh->background[size_cls]))) {
        ch = list_entry(lh->background[size_cls].next, chunkh_t, list);
        list_del(&ch->list);
        goto finish;
    }

    // 从local free list中获取chunk
    if (!list_empty(&lh->free_list)) {
        ch = list_entry(lh->free_list.next, chunkh_t, list);
        list_del(&ch->list);
        // 将获取的chunk转换成所需要的大小
        goto initchunk;
    }

    // 从gpoll中取得chunk,并转换chunk为所需要的大小
    ch = gpool_acquire_chunk();

    ch->owner = lh;
initchunk:
    chunk_init(ch, size_cls);

finish:
    lh->foreground[size_cls] = ch;
    ch->state = FORG;
}

static void remote_block_collect(lheap_t *lh) {
    pthread_mutex_lock(&lh->lock);
    list_head *block;
    list_for_each(block, &lh->remote_free){
        list_del(block);
        chunkh_t *ch = chunk_extract_header(block);
        chunk_free_small(ch, block);
    }
    pthread_mutex_unlock(&lh->lock);
}

static void chunk_free_small(chunkh_t *ch, void *ptr) {
    lheap_t *lh = local_heap;
    lheap_t *target_lh = ch->owner;
    /* remote free */
    if (target_lh != lh){
        pthread_mutex_lock(&target_lh->lock);
        /* 
        延迟回收,只要简单地将block地址放入remote free list即可,无需修改
        chunk status和其它chunk中的数据,这些状态将会由chunk所属的线程
        根据时机回收,这样可以减少对线程加锁的开销
         */
        list_add_tail((list_head *)ptr, &target_lh->remote_free);
        pthread_mutex_unlock(&target_lh->lock);
    }
    /* local free */
    else{
        dlist_add(&ch->dlist_head, (dlist_t *)ptr);
        ch->free_tot_cnt++;

        int size_cls = ch->size_cls;
        lheap_t *lh = ch->owner;

        switch (ch->state) {
            /* 无需做任何事 */
            case FORG:
                break;

            /* 当chunk已满,则将chunk state置为BACK并将chunk放到background list中 */
            case FULL:
                list_add_tail(&ch->list, &lh->background[size_cls]);
                ch->state = BACK;
                break;
            /* 当一个chunk的block已经被完全释放并且该chunk在background中,则将该chunk放入local heap freelist */
            case BACK:  
                if (unlikely(ch->free_tot_cnt == ch->blk_cnt)) {
                    list_del(&ch->list);
                    list_add_tail(&ch->list, &lh->free_list);
                }
                break;

            /* chunk state 错误 */
            default:
                fprintf(stderr, "fatal error: unknown state: %d\n", ch->state);
                exit(-1);
        }
    }
}

static void chunk_free_large(chunkh_t *ch) {
    page_free(ch->free_mem, ch->blk_size);
}

static void *small_malloc(int size_cls) {
    lheap_t *lh = local_heap;
    chunkh_t *ch;
    void *ret = NULL;
    /* 尝试从foreground chunk中分配内存 */
    ch = lh->foreground[size_cls];
    /* 尝试回收remote free list中的内存块 */
    if(ch->free_tot_cnt == 0){
        remote_block_collect(lh);
    }
    /* 替换已经满了的foreground chunk,并重新malloc */
    if(ch->free_tot_cnt == 0 || ch->size_cls == DUMMY_CLASS){
        ch->state = FULL;
        /* 将foreground chunk放到background并从后台调出块到foreground */
        lheap_replace_foreground(lh, size_cls);
    }

    
    /* 从chunk中切分出所需要的block */
    ch = lh->foreground[size_cls];
    ret = chunk_alloc_obj(ch);

    /* 一段时间未使用之后归还到global pool */
    list_head *chunk_ptr;
    list_for_each(chunk_ptr, &lh->free_list){
        chunkh_t *free_chunk = list_entry(chunk_ptr, chunkh_t, list);
        if (unlikely(free_chunk->spare_count)>=WEAR_LIMIT){
            pthread_mutex_lock(&gpool.lock);
            list_add_tail(&ch->list, &gpool.free_list);
            pthread_mutex_unlock(&gpool.lock);
        }
        else
            free_chunk->spare_count++;
    }

    return ret;
}

static void *large_malloc(size_t size) {
    size_t size_tot = size + sizeof(chunkh_t) + CHUNK_SIZE;

    /* >65536B大内存分配直接调用mmap */
    void *start_mem = page_alloc(NULL, size_tot);
    void *ret = (void *)ROUNDUP((uint64_t)start_mem, CHUNK_SIZE);
    chunkh_t *ch = (chunkh_t *)ret;
    ch->owner = (lheap_t *)LARGE_OWNER;
    ch->free_mem = start_mem;
    ch->blk_size = size_tot;
    
    return ret + sizeof(chunkh_t);
}

static void gpool_grow() {
    /* 全局堆空间用完，调用mmap进行增长 */
    void *ret = page_alloc(gpool.pool_end, ALLOC_UNIT);
    if (ret < 0) {
        fprintf(stderr, "page_alloc failed\n");
        exit(-1);
    }

    gpool.pool_end += ALLOC_UNIT;
}

inline static chunkh_t *gpool_acquire_chunk() {
    pthread_mutex_lock(&gpool.lock);

    chunkh_t *ch = gpool.free_start;
    do {
        /* 优先分配未使用过的chunk */
        if (gpool.free_start + CHUNK_SIZE <= gpool.pool_end) {
            gpool.free_start += CHUNK_SIZE;
            break;
        }

        /* 如果没有未使用过的chunk，则从空闲链表中获取 */
        if (!list_empty(&gpool.free_list)){
            ch = list_entry(gpool.free_list.next, chunkh_t, list);
            list_del(&ch->list);
            break;
        }

        /* 没有可用的chunk，则扩张全局堆空间 */
        gpool_grow();
        gpool.free_start += CHUNK_SIZE;
    } while(0);

    pthread_mutex_unlock(&gpool.lock);

    return ch;
}

inline static int size2cls(size_t size) {

    /* 根据size来快速获得大小类的索引 */
    int ret;
    if (likely(size <= 1024)) {
        ret = sizemap[(size - 1) >> 3];
    } else if (size <= 65536) {
        ret = sizemap2[(size - 1) >> 9];
    } else {
        ret = LARGE_CLASS;
    }

    return ret;
}

void *tr_malloc(size_t size) {
    void *ret = NULL;
    check_init();
    #ifndef NTMR
    /* 简单三模冗余,直接分配三倍大小的内存,但对于用户来说只被分配到size大小数据 */
    size = size * 3;    
    #endif
    size += (size == 0);

    
    int size_cls = size2cls(size);
    /* 小内存分配 */
    if (likely(size_cls < DEFAULT_BLOCK_CLASS)) {
        ret = small_malloc(size_cls);
    } 
    /* 大内存分配 */
    else if (size_cls == LARGE_CLASS) {
        ret = large_malloc(size);
    } 
    /* 分配失败 */
    else {
        fprintf(stderr, "fatal error: unknown class %d\n", size_cls);
        exit(-1);
    }

    return ret;
}


void tr_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    chunkh_t *ch = chunk_extract_header(ptr);
    lheap_t *lh = local_heap;
    lheap_t *target_lh = ch->owner;

    /* 小内存释放 */
    if ((uint64_t)target_lh != LARGE_OWNER) {
        chunk_free_small(ch, ptr);
    } 
    /* 大内存释放 */
    else {
        chunk_free_large(ch);
    }
}

static void *page_alloc(void *pos, size_t size) {
    return mmap(pos, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

static void page_free(void *pos, size_t size) {
    munmap(pos, size);
}

static lheap_t *acquire_lheap() {
    pthread_mutex_lock(&heap_pool.lock);

    lheap_t *lh = heap_pool.free_start;
    /* 从本地堆结构池中获取一个本地堆结构体 */
    /* 跟获取chunk的流程相同 */
    do {
        if (heap_pool.free_start + LHEAP_SIZE <= heap_pool.pool_end) {
            heap_pool.free_start += LHEAP_SIZE;
            break;
        }
        if (!list_empty(&heap_pool.free_list)){
            lh = list_entry(heap_pool.free_list.next, lheap_t, list);
            list_del(&lh->list);
            break;
        }

        heap_pool_grow();
        heap_pool.free_start += LHEAP_SIZE;
    } while(0);

    pthread_mutex_unlock(&heap_pool.lock);
    
    /* local heap的初始化处理,包括清空数据结构中的各项内容,并初始化mutex */
    int i;
    for (i=0; i<DEFAULT_BLOCK_CLASS; i++) {
        lh->foreground[i] = &(lh->dummy_chunk);
        INIT_LIST_HEAD(&(lh->background[i]));
    }
    INIT_LIST_HEAD(&lh->free_list);
    INIT_LIST_HEAD(&lh->remote_free);

    lh->dummy_chunk.owner = lh;
    lh->dummy_chunk.size_cls = DUMMY_CLASS;
    lh->dummy_chunk.blk_size = 0;
    lh->dummy_chunk.blk_cnt = 0;
    lh->dummy_chunk.free_mem_cnt = 0;
    lh->dummy_chunk.free_tot_cnt = 1;
    lh->dummy_chunk.free_mem = (void *)0;
    dlist_init(&lh->dummy_chunk.dlist_head, &lh->dummy_chunk.dlist_tail);
    if (pthread_mutex_init(&lh->lock, NULL) < 0 ) {
        fprintf(stderr, "fatal error: pthread_mutex_init failed\n");
        exit(-1);
    }

    return lh;
}

static void heap_pool_grow(){
    void *ret = page_alloc(heap_pool.pool_end, LHEAP_UNIT);
    if (ret < 0) {
        fprintf(stderr, "page_alloc failed\n");
        exit(-1);
    }

    heap_pool.pool_end += LHEAP_UNIT;
}

static void *tr_calloc(size_t nelem, size_t elsize){
    /* 在内存的动态存储区中分配num个长度为size的连续空间 */
    register void *ptr;
    if (nelem == 0 || elsize == 0)
        nelem = elsize = 1;
    ptr = tr_malloc(nelem * elsize);
    if (ptr)
        bzero(ptr, nelem * elsize);
    /* 
     * 函数返回一个指向分配起始地址的指针函数返回一个指向分配起始地址的指针
     * 如果分配不成功，返回NULL
     */
    return ptr;
}


//参考ptmalloc
static void *tr_realloc(void *ptr, size_t size){

    void *new = NULL;
    /* realloc of null is supposed to be same as malloc */
    if (ptr == NULL){
        return tr_malloc(size);
    }

    //REALLOC_ZERO_BYTES_FREES
    if(size==0){
        tr_free(ptr);
        return NULL;
    }

    chunkh_t *ch = chunk_extract_header(ptr);
    if(size == ch->blk_size){
        return ptr;
    }
    new = tr_malloc(size);
    memcpy(ptr, new, ch->blk_size);
    return new;
}

#ifndef NTMR
// 对有异常的内存，利用三模冗余的机制自动修复，并返回指向正确的值的指针
void * TR_Read(void* ptr){
   
    chunkh_t* p = chunk_extract_header(ptr);   // 根据传入的地址来获得此地址所在chunk的首地址
    uint64_t temp = (uint64_t)ptr - (uint64_t)p - sizeof(chunkh_t); // 传入的ptr 减去 chunk首地址然后再减去chunk头部的大小，即获得ptr与第一个block的首地址的距离temp
    // temp减去temp%blk_size，即减掉了ptr在所在block中的偏移，然后再加上chunk首地址和chunk头部大小，即得到ptr所在block的首地址
    void * bptr =  (void *)((uint64_t)temp - (uint64_t)(temp) % p->blk_size + (uint64_t)p + sizeof(chunkh_t)) ;
   
    uintptr_t b_size = p->blk_size / 3;  // b_size 是三模冗余中三分之一块的大小，即存放content的大小
    void * b2ptr = (void*)((uint64_t)bptr + b_size); // bptr + b_size 得到第一个冗余块的首地址b2ptr
    void * b3ptr = (void*)((uint64_t)b2ptr + b_size);  // b2ptr + b_size 得到第二个冗余块的首地址b3ptr

    // compare数组，初始化为0，表示三次对比的结果，即1与2，1与3，2与3的比较，为0表示两块内容相同
    int compare[3] = {0, 0, 0};
    if(memcmp(bptr, b2ptr, b_size)){ // 1与2对比，内容不同置为1
        compare[0] = 1;
    }
    if(memcmp(bptr, b3ptr, b_size)){// 1与3对比，内容不同置为1
        compare[1] = 1;
    }
    if(memcmp(b2ptr, b3ptr, b_size)){// 2与3对比，内容不同置为1
        compare[2] = 1;
    }

    int flag = 0; // flag为0表示三块都相同 为1表示有一块不同
    int index = -1; // index指向三次比较中两块都相同的一次比较
    for(int i=0; i<3; i++){
        if(compare[i] == 1){
            flag = 1;
        }
        if(compare[i] == 0){
            index = i;
        }
    }
    // 三块都相同，不需要做恢复，直接返回ptr
    if(flag == 0){
        return ptr;
    }
    // 三次比较都不同，不止一个bit翻转，出错
    if(index == -1){
        return NULL;
    }
    // 有一个块发生了翻转
    if(index == 0){                     // 1与2 相同，说明第3块发生翻转，用1的内容覆盖3
        memcpy(b3ptr, bptr, b_size);
    }else if(index == 1){               // 1与3 相同，说明第2块发生翻转，用1的内容覆盖2
        memcpy(b2ptr, bptr, b_size);
    }else{                              // 2与3 相同，说明第1块发生翻转，用2的内容覆盖1
        memcpy(bptr, b2ptr, b_size);
    }
    return ptr;
    
}

// 传入目的地址，源地址，写入字节数，然后同时写入三个块中对应地址。
// 在写时不需要关注单bit翻转，因为翻转的有可能会被覆盖掉，而且没被覆盖也能在读的时候检测出来。
intptr_t TR_Write(void* ptr, void* source, size_t size){
    chunkh_t* p = chunk_extract_header(ptr);   // 根据传入的地址来获得此地址所在chunk的首地址
    uint64_t temp = (uint64_t)ptr - (uint64_t)p - sizeof(chunkh_t); // 传入的ptr 减去 chunk首地址然后再减去chunk头部的大小，即获得ptr与第一个block的首地址的距离temp
    // temp减去temp%blk_size，即减掉了ptr在所在block中的偏移，然后再加上chunk首地址和chunk头部大小，即得到ptr所在block的首地址
    void * bptr =  (void *)((uint64_t)temp - (uint64_t)(temp) % p->blk_size + (uint64_t)p + sizeof(chunkh_t)) ;
   
    uintptr_t b_size = p->blk_size / 3;  // b_size 是三模冗余中三分之一块的大小，即存放content的大小
    void * b2ptr = (void*)((uint64_t)bptr + b_size); // bptr + b_size 得到第一个冗余块的首地址b2ptr
    void * b3ptr = (void*)((uint64_t)b2ptr + b_size);  // b2ptr + b_size 得到第二个冗余块的首地址b3ptr

    void * ptr2 = (void *)((uintptr_t)b2ptr + (uintptr_t)ptr - (uintptr_t)bptr);  // 获得第一个冗余块中要写入的开始地址

    void * ptr3 = (void *)((uintptr_t)b3ptr + (uintptr_t)ptr - (uintptr_t)bptr);  // 获得第二个冗余块中要写入的开始地址

    // 将写入的值同时写入第一块，第二块和第三块对应位置
    memcpy(ptr, source, size); 
    memcpy(ptr2, source, size);
    memcpy(ptr3, source, size);
    
    return 1;

}
#endif
