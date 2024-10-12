#include<stdlib.h>

#include"queue.h"

typedef struct __queue_node{
    struct __queue_node* next;
    int data;
}__queue_node_t;

queue_t* queue_create(){
    queue_t* q = malloc(sizeof(queue_t));
    if(q == NULL) return NULL;
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    pthread_mutex_init(&q->lock, NULL);
    return q;
}
void queue_destroy(queue_t* q){
    if(q == NULL){
        return;
    }
    while(q->size){
        __queue_node_t* temp = q->head;
        q->head = q->head->next;
        free(temp);
        q->size--;
    }
    free(q);
}
void queue_enqueue(queue_t* q, int data){
    __queue_node_t* node = malloc(sizeof(__queue_node_t));
    if(node == NULL){
        return;
    }
    node->data = data;
    node->next = NULL;
    pthread_mutex_lock(&q->lock);
    if(q->tail == NULL){
        q->head = node;
        q->tail = node;
    }else{
        q->tail->next = node;
        q->tail = node;
    }
    q->size++;
    pthread_mutex_unlock(&q->lock);
}
int queue_dequeue(queue_t* q){
    int data = 0;
    pthread_mutex_lock(&q->lock);
    if(q->head == NULL){
        pthread_mutex_unlock(&q->lock);
        return 0;
    }
    data = q->head->data;
    __queue_node_t* temp = q->head;
    q->head = q->head->next;
    if(q->head == NULL){
        q->tail = NULL;
    }
    free(temp);
    q->size--;
    pthread_mutex_unlock(&q->lock);
    return data;
}