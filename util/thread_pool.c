/*
you can only use -O0 to compile this file with gcc
*/


#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/time.h>
#include<errno.h>
#include<signal.h>

#include "thread_pool.h"
#include "../log/log.h"

#define LIM_MAX_THRD 30000

struct __thread_pool_t{
    thrd_pool_work_t *work_head;
    thrd_pool_work_t *work_end;

    pthread_t *tid;
    pthread_t tadmin;
    pthread_t init_thrd;        //creater thread id
    pthread_mutex_t lock;
    pthread_attr_t attr;

    size_t max_thrd;
    size_t busy_thrd;
    size_t work_count;

    int exit;
    int admin_exit;
};

void __thrd_exit(int sig){
    if(sig == SIGINT || sig == SIGTERM){
        pthread_exit(NULL);
    }
}

long __get_cur_ms(){          
   struct timeval tv;                      
   gettimeofday(&tv, NULL);                
   return tv.tv_sec*1000 + tv.tv_usec/1000;
} 

void *__create_thread(void *arg){
    long tm, wait_tm = 0, timeout = 10;
    pthread_t tid = pthread_self();
    
    
    #ifdef DEBUG
    log_debug(LOG_USE_FILE_LINE, "Thread %u start", tid);
    #endif //DEBUG

    //get thread arg
    thrd_pool_t *tpool = (thrd_pool_t*)arg;

    thrd_pool_work_t *work = NULL;

    while(1){

        //wait for work
        //if no work, wait for timeout
        tm = __get_cur_ms();
        while(wait_tm <= timeout && !tpool->work_count && !tpool->exit){
            usleep(10);
            wait_tm = __get_cur_ms() - tm;
        }

        //if exit, exit thread
        if(tpool->exit){
        #ifdef DEBUG
            log_debug(LOG_USE_FILE_LINE, "Thread %u exit", tid);
        #endif //DEBUG
            break;
        }
        //if no work, exit thread
        if(wait_tm >= timeout){
        #ifdef DEBUG
            log_debug(LOG_USE_FILE_LINE, "Thread %u wait timeout", tid);
            log_debug(LOG_USE_FILE_LINE, "wait_tm: %lu, tm: %lu, now: %lu", wait_tm, tm, __get_cur_ms());
        #endif //DEBUG
            break;
        }

        //get work
    #ifdef DEBUG
        clock_t start_tm = clock(); //start time
    #endif
        pthread_mutex_lock(&tpool->lock);
        if(!tpool->work_count){
            pthread_mutex_unlock(&tpool->lock);
            continue;
        }
        work = tpool->work_head;
        tpool->work_head = tpool->work_head->next;
        tpool->work_count--;

        pthread_mutex_unlock(&tpool->lock);

    #ifdef DEBUG
        clock_t end_tm = clock(); //end time
        //calc time
        double time_used = (double)(end_tm - start_tm) / CLOCKS_PER_SEC;
        log_debug(LOG_USE_FILE_LINE, "Thread %u get work time: %f", tid, time_used);
    #endif //DEBUG

        //run work
        work->func(work->arg);
        free(work);
    }
    
    
    
    int pos;

    for (int lpos = 0, rpos = tpool->max_thrd - 1; lpos < rpos; lpos++, rpos--){    
        if(tpool->tid[lpos] == tid){
            pos = lpos;
            break;
        }
        if(tpool->tid[rpos] == tid){
            pos = rpos;
            break;
        }
    }

    //set thread id to 0
    pthread_mutex_lock(&tpool->lock);
    tpool->tid[pos] = 0;
    tpool->busy_thrd--;
    pthread_mutex_unlock(&tpool->lock);

#ifdef DEBUG
    log_debug(LOG_USE_FILE_LINE, "Thread %u exit\n\n", tid);
#endif //DEBUG
    //exit thread
    pthread_exit(NULL);
}

void *__admin_thread(void *arg){

    //get thread pool
    thrd_pool_t *tpool = (thrd_pool_t*)arg;
    int work_thrd;

    while(1){
        //get busy threads
        for(int i = 0; i < tpool->max_thrd; i++){
            if(tpool->tid[i] == 0){
                work_thrd = i;
                break;
            }
        }

        //create new thread if work count is more than busy threads
        if(tpool->work_count > tpool->busy_thrd*2 &&
         tpool->busy_thrd < tpool->max_thrd){

        #ifdef DEBUG    
            clock_t start_tm = clock(); //start time
        #endif //DEBUG

            if(pthread_create(&tpool->tid[work_thrd], &tpool->attr, __create_thread, (void*)tpool)){
                log_error(LOG_USE_FILE_LINE, "Can't create thread: Caller=>%p", __builtin_return_address(0));
                log_error(LOG_USE_FILE_LINE,"%s", strerror(errno));
                exit(-1);
            }

            //increase busy threads
            pthread_mutex_lock(&tpool->lock);
            tpool->busy_thrd++;
            pthread_mutex_unlock(&tpool->lock);

        #ifdef DEBUG    
            clock_t end_tm = clock(); //end time

            //calc time
            double time_used = (double)(end_tm - start_tm) / CLOCKS_PER_SEC;
            log_debug(LOG_USE_FILE_LINE, "Create thread time: %f", time_used);
        #endif //DEBUG

            continue;
        }

        if(tpool->admin_exit) break;

        usleep(1000); //wait 1ms
    }
    tpool->tadmin = 0;
    pthread_exit(NULL);
}

int thrdpool_create(size_t max_thrd, thrd_pool_t **tpool){
    
    if(max_thrd > LIM_MAX_THRD){
        log_error(LOG_USE_FILE_LINE, "Too many threads: %d", max_thrd);
        return -1;
    }

    int ret;
    
    // Create thread pool
    *tpool = malloc(sizeof(struct __thread_pool_t));
    if(!(*tpool)){
        log_warn(LOG_USE_FILE_LINE, "Can't malloc: Caller=>%p", __builtin_return_address(0));
        return -1;
    }
    // Init thread pool
    memset(*tpool, 0, sizeof(thrd_pool_t));

    //set max threads
    (*tpool)->max_thrd = max_thrd;

    // Create thread id array
    (*tpool)->tid = malloc(sizeof(pthread_t*) * (*tpool)->max_thrd);
    if(!(*tpool)->tid){
        log_warn(LOG_USE_FILE_LINE, "Can't malloc: Caller=>%p", __builtin_return_address(0));
        free(*tpool);
        return -1;
    }
    memset((*tpool)->tid, 0, sizeof(pthread_t)*(*tpool)->max_thrd);

    //set create thread id
    (*tpool)->init_thrd = pthread_self();

    //init mutex
    pthread_mutex_init(&(*tpool)->lock, NULL);
    pthread_mutex_init(&(*tpool)->lock, NULL);

    //init attr
    pthread_attr_init(&(*tpool)->attr);
    pthread_attr_setdetachstate(&(*tpool)->attr, PTHREAD_CREATE_DETACHED);

    //create admin thread
    ret = pthread_create(&(*tpool)->tadmin, &(*tpool)->attr, __admin_thread, (void*)(*tpool));
    if(ret){
        log_warn(LOG_USE_FILE_LINE, "Can't create admin thread: Caller=>%p", __builtin_return_address(0));
        log_error(LOG_USE_FILE_LINE,"%s", strerror(errno));
        exit(-1);
    }

    return 0;
}

int thrdpool_add_work(thrd_pool_work_t work, thrd_pool_t *tpool){

    //create new work
    thrd_pool_work_t *new_work = malloc(sizeof(thrd_pool_work_t));
    if(!new_work){
        log_warn(LOG_USE_FILE_LINE, "Can't malloc: Caller=>%p", __builtin_return_address(0));
        return -1;
    }

    memcpy(new_work, &work, sizeof(thrd_pool_work_t));

    //add work to thread pool
    pthread_mutex_lock(&tpool->lock);
    new_work->next = NULL;
    if(tpool->work_head == NULL){
        tpool->work_head = new_work;
        tpool->work_end  = new_work;
    }else{
        tpool->work_end->next = new_work;
        tpool->work_end = new_work;
    }
    tpool->work_count++;
    pthread_mutex_unlock(&tpool->lock);

    return 0;
}

int thrdpool_exit(int flag, thrd_pool_t **tpool){

    pthread_t self_tid = pthread_self();
    int is_init_thrd = self_tid == (*tpool)->init_thrd ? 1 : 0;
    int in_thrdpool = 0;
    int pos = 0;
    for(; pos < (*tpool)->max_thrd; pos++){
        if((*tpool)->tid[pos] == self_tid){
            in_thrdpool = 1;
            break;
        }
    }

    if((*tpool)->busy_thrd > 0 && !is_init_thrd && in_thrdpool){
        (*tpool)->tid[pos] = 0;
        (*tpool)->busy_thrd--;
    }

    switch (flag){
    case THRDPOOL_EXIT_NONE:
        (*tpool)->exit = 1;
        pthread_mutex_lock(&(*tpool)->lock);
        //clear work queue
        while((*tpool)->work_head){
            thrd_pool_work_t *tmp = (*tpool)->work_head;
            (*tpool)->work_head = (*tpool)->work_head->next;
            free(tmp);
            (*tpool)->work_count--;
        }
        pthread_mutex_unlock(&(*tpool)->lock);
        
        while((*tpool)->busy_thrd){
            for(int pos = 0; pos < (*tpool)->max_thrd && !(*tpool)->tid[pos]; pos++);
            usleep(10 * 1000);
        }

        //exit admin thread
        (*tpool)->admin_exit = 1;

        while((*tpool)->tadmin){
            usleep(1000);
        }

        break;

    case THRDPOOL_EXIT_WAIT:

        //wait for all threads exit
        while ((*tpool)->work_head || (*tpool)->busy_thrd){
            for(int pos = 0; pos < (*tpool)->max_thrd && !(*tpool)->tid[pos]; pos++);
            usleep(10 * 1000);
        }

        //exit admin thread
        (*tpool)->admin_exit = 1;
        while ((*tpool)->tadmin){
            usleep(1000);
        }

        break;

    default:
        break;
    }

    free((*tpool)->tid);
    free(*tpool);

    *tpool = NULL;

    if(in_thrdpool){
        pthread_exit(NULL);
    }
    
    return 0;
}