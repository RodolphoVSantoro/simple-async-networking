#include <stdio.h>
#include <stdlib.h>
#include <aio.h>
#include <string.h>
#include <errno.h>

#define BUFFSIZE 256

struct aiocb * async_read(FILE *fp, char *buff, int bytes) {
    struct aiocb *aio = (struct aiocb *)malloc(sizeof(struct aiocb));
    if(!aio) {
        return NULL;
    }

    memset(aio, 0, sizeof(*aio));
    
    aio->aio_buf = buff;
    aio->aio_fildes = fileno(fp);
    aio->aio_nbytes = bytes;
    aio->aio_offset = 0;

    int result = aio_read(aio);
    if(result < 0) { 
        free(aio);
        return NULL;
    }

    return aio;
}

int main(){
    FILE *fp = fopen("test.txt", "r");
    FILE *fp2 = fopen("test2.txt", "r");
    if(fp == NULL || fp2 == NULL) { 
        return -1;
    }

    char buff[BUFFSIZE];
    char buff2[BUFFSIZE];

    struct aiocb *aio = async_read(fp, buff, BUFFSIZE);
    struct aiocb *aio2 = async_read(fp, buff2, BUFFSIZE);

    if(aio == NULL || aio2 == NULL) {
        fclose(fp);
        fclose(fp2);
        return -1;
    }

    int count = 0;
    while(aio_error(aio) == EINPROGRESS || aio_error(aio2) == EINPROGRESS) {
        count++;
    }

    int ret = aio_return(aio);
    
    fclose(fp);
    fclose(fp2);

    printf("Read %d bytes\n", ret);
    printf("Looped %d times\n", count);
    return 0;
}