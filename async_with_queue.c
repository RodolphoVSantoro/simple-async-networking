#include <stdio.h>
#include <stdlib.h>
#include <aio.h>
#include <string.h>
#include <errno.h>
#include "aioQueue.h"

#define BUFFSIZE 2048 * 128

int main(){
    FILE *fp = fopen("test.txt", "r");
    FILE *fp2 = fopen("test2.txt", "r");
    if(fp == NULL || fp2 == NULL) { 
        return -1;
    }

    AioQueue queue;
    aioQueueInit(&queue);

    char buff[BUFFSIZE];
    char buff2[BUFFSIZE];

    int pushResult1 = aioQueuePushFile(&queue, fp, buff, BUFFSIZE);
    int pushResult2 = aioQueuePushFile(&queue, fp2, buff2, BUFFSIZE);

    if(pushResult1 != 0 || pushResult2 != 0) {
        return -1;
    }

    int count = 0;
    int readFiles = 0;

    while(readFiles < 2) {
        Aiocb* doneFile = getNextDone(&queue);
        if(doneFile != NULL) {
            readFiles++;
            int fileResult = aio_return(doneFile);
            free(doneFile);
            printf("Read %d bytes\n", fileResult);
        }
        count++;
    }

    fclose(fp);
    fclose(fp2);

    printf("Looped %d times\n", count);
    return 0;
}