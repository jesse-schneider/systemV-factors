#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <string.h>

#include "memory.h"

void * factoring(void *);
unsigned int rotateRight(unsigned int);

void * factoring(void * arg) {
    /** Trial By Division Factorisation **/

    unsigned int num = (int) arg;
    int maybeFactor = 2; 
  
    // Computing the square root of num
    int root = ceil(sqrt(num)); 
  
    // While loop < square root
    while(maybeFactor <= root) { 
  
        // If == 0, then is a factor 
        if(num % maybeFactor == 0)
            printf("factor: %d\n", maybeFactor); //this is when we send it back to the client
        maybeFactor++; 
    } 
    return NULL;
}


unsigned int rotateRight(unsigned int num) {
    printf("num before rotating: %u\n", num);
    //rightShift = 0x12345678 >> 4 = 0x01234567
    unsigned int rightShift = num >> 1;

    //lsb = (0x12345678 << 28) = 0x80000000
    unsigned int lsb = num << 31;

    unsigned int combined = rightShift | lsb;
    return combined;
}

void multithreading(unsigned int number) {

    int num_threads = 32;
    pthread_t threads[num_threads];
    
    //create threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, factoring, (void *) &number);
        // sleep(1);
        number = rotateRight(number);
    }
    

    //join completed threads back
    for (int i = 0; i < num_threads; i++) {
        void * result;
        // sleep(1);
        pthread_join(threads[i], NULL);
    }
}


void main(void) {

    //init variables
    key_t shmKey;
    int shmId;
    struct MemoryStruct * memptr;

    //shared memory get function to get shared memory using unique key (made with ftok)
    shmKey = ftok(".", 'x');
    shmId = shmget(shmKey, sizeof(struct MemoryStruct), 0666);
    if(shmId < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    printf("server has recieved shared memory...\n");

    memptr = (struct MemoryStruct *) shmat(shmId, NULL, 0);
    if((int) memptr == -1) {
        printf("*** shamt error (server) ***\n");
        exit(1);
    }

    printf("server has attached the shared memory...\n");

    while(memptr->clientflag != FULL)
        ;
    unsigned int num = memptr->number;

    for(int i = 0; i < 10; i++) {
        printf("flag: %c\n", memptr->serverflag[i]);
        if(memptr->serverflag[i] == '0') {
            printf(" slot found: %d\n", i);
            memptr->number = i;
            multithreading(num);
            memptr->clientflag = EMPTY;
            break;
        }
    }
    
    

    printf("server has informed client array has been taken...\n");
    shmdt((void *) memptr);
    printf("server has detached it's shared memory...\n");
    printf("server exits...\n");
    exit(0);
}
