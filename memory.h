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
#define EMPTY 0
#define FULL 1
#define COMPLETE 2


typedef struct MemoryStruct {
    char clientflag;
    unsigned int number;
    char serverflag[10];
    int slots[10];
    pthread_mutex_t client;
    pthread_mutex_t numMutex;
    pthread_mutex_t server[10];
} MemoryStruct;