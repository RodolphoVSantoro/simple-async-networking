#ifndef AIO_QUEUE_H
#define AIO_QUEUE_H

#include <aio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct aiocb Aiocb;
typedef Aiocb* AiocbPtr;

#define MAX_AIO_QUEUE_SIZE 100
#define moveRight(index) (index = (index + 1) % MAX_AIO_QUEUE_SIZE)

typedef struct AIO_QUEUE {
    AiocbPtr elements[MAX_AIO_QUEUE_SIZE];
    int start;
    int end;
} AioQueue;

void aioQueueInit(AioQueue *queue){
    queue->start = 0;
    queue->end = 0;
    for(int i = 0; i < MAX_AIO_QUEUE_SIZE; i++){
        queue->elements[i] = NULL;
    }
}

int aioQueuePushFile(AioQueue *queue, FILE *fp, char *buff, int bytes){
    if(queue->end == queue->start && queue->elements[queue->end] != NULL){
        return -1;
    }

    Aiocb *aiocb = (Aiocb *)malloc(sizeof(Aiocb));
    if(!aiocb) {
        return -1;
    }

    memset(aiocb, 0, sizeof(*aiocb));
    
    aiocb->aio_buf = buff;
    aiocb->aio_fildes = fileno(fp);
    aiocb->aio_nbytes = bytes;
    aiocb->aio_offset = 0;

    int result = aio_read(aiocb);
    if(result < 0) { 
        free(aiocb);
        return -1;
    }

    queue->elements[queue->end] = aiocb;
    moveRight(queue->end);

    return 0;
}

void tryQueueClear(AioQueue *queue) {
    while(queue->start != queue->end && queue->elements[queue->start] == NULL){
        moveRight(queue->start);
    }
}

int queueLen(AioQueue *queue) {
    if(queue->start <= queue->end){
        return queue->end - queue->start;
    }
    return MAX_AIO_QUEUE_SIZE - queue->start + queue->end;
}

/*
    returns the next done Aiocb in the queue
    returns NULL if there are no done Aiocb in the queue
*/
Aiocb* getNextDone(AioQueue *queue){
    if(queue->start == queue->end){
        return NULL;
    }
    int current = queue->start;
    while(current != queue->end){
        Aiocb *aio = queue->elements[current];
        if(aio != NULL && aio_error(aio) == 0){
            queue->elements[current] = NULL;
            return aio;
        }
        moveRight(current);
    }
    return NULL;
}

void freeAll(AioQueue *queue){
    for(int i = 0; i < MAX_AIO_QUEUE_SIZE; i++){
        if(queue->elements[i] != NULL){
            free(queue->elements[i]);
            queue->elements[i] = NULL;
        }
    }
    queue->start = 0;
    queue->end = 0;
}

#endif