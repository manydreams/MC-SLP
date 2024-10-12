#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include<pthread.h>

typedef struct thrdpool_work_t{
    void *arg;
    void (*func)(void *args);
    struct thrdpool_work_t *next;
}thrdpool_work_t;

typedef struct __thread_pool_t thrdpool_t;

enum{THRDPOOL_EXIT_NONE,THRDPOOL_EXIT_WAIT};

/*
创建线程池，成功返回0，失败返回-1
create thread pool, return 0 on success, -1 on failure

tpool: 指向线程池的指针，成功创建后会被赋值
tpool: pointer to thread pool, will be assigned after successful creation

max_thrd: 线程池中最大线程数
max_thrd: maximum number of threads in the thread pool
*/
int thrdpool_create(size_t max_thrd, thrdpool_t **tpool);


/*
向线程池中添加工作项，成功返回0，失败返回-1
add work to thread pool, return 0 on success, -1 on failure

work: 工作项
work: work item

tpool: 指向线程池的指针
tpool: pointer to thread pool
*/
int thrdpool_add_work(thrdpool_work_t work, thrdpool_t *tpool);

/*
退出线程池
exit thread pool
*/
int thrdpool_exit(int flag, thrdpool_t **tpool);

#endif