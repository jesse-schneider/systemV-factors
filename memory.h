#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <semaphore.h>
#include <fcntl.h>
#define EMPTY 0
#define FULL 1
#define COMPLETE 2
#define NUM_SLOTS 10
#define NUM_THREADS 32



typedef struct MemoryStruct {
    char clientflag;
    unsigned int number;
    int serverflag[NUM_SLOTS];
    int slots[NUM_SLOTS];
    pthread_cond_t clientCond;
    pthread_mutex_t client;
    pthread_mutex_t server[NUM_SLOTS];
    pthread_cond_t serverCond[NUM_SLOTS];
} MemoryStruct;



/******** Server/Producer Prototypes ********/
void * factorNumber(void *);
unsigned int rotateRight(unsigned int);
void * handleQuery(void *);


/******** Client/Consumer Prototypes ********/
void * processQuery(void *);
void * displayProgress(void *);
