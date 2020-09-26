

#include "memory.h"

void * factoring(void *);
unsigned int rotateRight(unsigned int);

typedef struct NumSlot {
    unsigned int number;
    int slot;
    MemoryStruct * memptr;
} NumSlot;


/** Trial By Division Factorisation **/
void * factoring(void * arg) {
    NumSlot * n = (NumSlot *) arg;
    int slot = (*n).slot;
    unsigned int num = (*n).number;
    MemoryStruct * memptr = (*n).memptr;
    
    int checkFactor = 2;
   
    // While loop < square root
    while(1) {
        usleep(5);
        // If == 0, then is a factor 
        if(num % checkFactor == 0) {
            while(memptr->serverflag[slot] != EMPTY) //{
            //     //all lock and unlock
            //     if == empty {
            //         lock
            //         bcbc
            //         unlock
            //     }
            // }
                ;
            printf("factor: %d\n", checkFactor); //this is when we send it back to the client
            pthread_mutex_lock(&memptr->server[slot]);
            memptr->slots[slot] = checkFactor;
            if((checkFactor) >= ceil(sqrt(num))) {
                memptr->serverflag[slot] = COMPLETE;
                break;
            } else {
                memptr->serverflag[slot] = FULL;
            }
        }
        checkFactor++;
    }
    
    printf("complete: %d\n", memptr->serverflag[slot]);
    return NULL;
}




void multithreads(unsigned int number, int slot,  MemoryStruct * memptr) {

    int num_threads = 32;
    pthread_t threads[num_threads];

    unsigned int rotatingNum = number;
    
    //create threads
    for (int i = 0; i < num_threads; i++) {
        
        NumSlot n = {rotatingNum, slot, memptr};
        pthread_create(&threads[i], NULL, factoring, (void *) &n);
        // sleep(1);
        rotatingNum = rotateRight(rotatingNum);
    }
    
    //join completed threads back
    for (int i = 0; i < num_threads; i++) {
        
        // sleep(1);
        pthread_join(threads[i], NULL);
    }
    printf("threads complete\n");
}


void main(void) {

    //init variables
    key_t shmKey;
    int shmId;
    MemoryStruct * memptr;

    //shared memory get function to get shared memory using unique key (made with ftok)
    shmKey = ftok(".", 'x');
    shmId = shmget(shmKey, sizeof(MemoryStruct), IPC_CREAT | 0666);
    printf("mem id: %d\n", shmId);
    if(shmId < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }

    memptr = (MemoryStruct *) shmat(shmId, NULL, 0);
    if((int) memptr == -1) {
        printf("*** shamt error (server) ***\n");
        exit(1);
    }
    printf("server has attached the shared memory...\n");

    //set shared memory to 0s
    for(int i = 0; i < 10; i++) {
        memptr->serverflag[i] = EMPTY;
        memptr->slots[i] = 0;
        pthread_mutex_init(&memptr->server[i], NULL);
    }
    memptr->clientflag = EMPTY;
    pthread_mutex_init(&memptr->client, NULL);
    

    for(;;) {

        while(memptr->clientflag != FULL)
            ;
        unsigned int num = memptr->number;

        for(int i = 0; i < 10; i++) {
            if(memptr->serverflag[i] == EMPTY) {
                printf(" slot found: %d\n", i);
                memptr->number = i;
                pthread_mutex_lock(&memptr->client);
                memptr->clientflag = EMPTY;
                multithreads(num, i, memptr);
                break;
            }
        }
    }

    printf("server has informed client array has been taken...\n");
    shmdt((void *) memptr);
    printf("server has detached it's shared memory...\n");
    printf("server exits...\n");
    exit(0);
}


unsigned int rotateRight(unsigned int num) {
    // printf("num before rotating: %u\n", num);
    //rightShift = 0x12345678 >> 4 = 0x01234567
    unsigned int rightShift = num >> 1;

    //lsb = (0x12345678 << 28) = 0x80000000
    unsigned int lsb = num << 31;

    unsigned int combined = rightShift | lsb;
    return combined;
}
