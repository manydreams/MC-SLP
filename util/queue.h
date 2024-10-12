#ifndef QUEUE_H
#define QUEUE_H

#include<pthread.h>

typedef struct __queue_node __queue_node_t;

typedef struct queue{
    __queue_node_t* head;
    __queue_node_t* tail;
    pthread_mutex_t lock;
    int size;
}queue_t;

queue_t* queue_create();
void queue_destroy(queue_t* q);
void queue_enqueue(queue_t* q, int data);
int queue_dequeue(queue_t* q);


#endif /* QUEUE_H */