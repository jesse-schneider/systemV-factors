

#include "memory.h"

void * factorNumber(void *);
unsigned int rotateRight(unsigned int);
void * handleQuery(void *);


typedef struct ServerQuery {
    unsigned int number;
    int slot;
    MemoryStruct * memptr;
    sem_t * queue;
} ServerQuery;


unsigned int rotateRight(unsigned int num) {
    return num >> 1 | num << 31;
}


/** Trial By Division Factorisation **/
void * factorNumber(void * threadQuery) {
    //extract variables from ServerQuery Struct
    ServerQuery * q = (ServerQuery *) threadQuery;
    int slot = (*q).slot;
    unsigned int num = (*q).number;
    MemoryStruct * memptr = (*q).memptr;
    
    int checkFactor = 2;
   
    // While loop < square root
    while((checkFactor) <= ceil(sqrt(num))) {
        // If == 0, then is a factor 
        if(num % checkFactor == 0) {
            pthread_mutex_lock(&memptr->server[slot]);
            while(memptr->serverflag[slot] != EMPTY)
                pthread_cond_wait(&memptr->serverCond[slot], &memptr->server[slot]);

            memptr->slots[slot] = checkFactor;
            memptr->serverflag[slot] = FULL;
            pthread_cond_signal(&memptr->serverCond[slot]);
            pthread_mutex_unlock(&memptr->server[slot]);
        }
        checkFactor++;
    }

    //thread has finished factoring, send COMPLETE signal to client
    pthread_mutex_lock(&memptr->server[slot]);
    while(memptr->serverflag[slot] != EMPTY)
        pthread_cond_wait(&memptr->serverCond[slot], &memptr->server[slot]);
        printf("complete: %d\n", memptr->serverflag[slot]);
    memptr->serverflag[slot] = COMPLETE;
    pthread_cond_signal(&memptr->serverCond[slot]);
    pthread_mutex_unlock(&memptr->server[slot]);
    return NULL;
}



void * handleQuery(void * query) {
    
    //extract variables from ServerQuery Struct
    ServerQuery * q = (ServerQuery *) query;
    int slot = (*q).slot;
    unsigned int rotatedNum = (*q).number;
    MemoryStruct * memptr = (*q).memptr;
    sem_t * queue = (*q).queue;

    //create pthread array for all 32 threads
    pthread_t threads[NUM_THREADS];
    
    //create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        ServerQuery n = {rotatedNum, slot, memptr, queue};
        pthread_create(&threads[i], NULL, factorNumber, (void *) &n);
        rotatedNum = rotateRight(rotatedNum);
    }
    
    //join completed threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("all threads complete\n");
    sem_post(queue);
    return NULL;
}


void main(void) {

    //memory variables
    key_t shmKey;
    int shmId;
    MemoryStruct * memptr;

    //server variables
    sem_t * queue;
    queue = sem_open("query", O_CREAT, 0666, 0);
    sem_init(queue, 0, NUM_SLOTS);
    pthread_t queries[NUM_SLOTS];
    pthread_mutexattr_t attrmutex;
    pthread_condattr_t attrcond;
    pthread_mutexattr_init(&attrmutex);
    pthread_condattr_init(&attrcond);

    //set mutexes for access cross-process
    if(pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED) != 0)
        perror("pthread_mutexattr_setpshared");
    if(pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED) != 0)
        perror("pthread_mutexattr_setpshared");

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

    //init client flag and client flag mutex/condition variable
    memptr->clientflag = EMPTY;
    pthread_mutex_init(&memptr->client, &attrmutex);
    pthread_cond_init(&memptr->clientCond, &attrcond);

    //set shared memory arrays to 0s
    for(int i = 0; i < NUM_SLOTS; i++) {
        memptr->serverflag[i] = EMPTY;
        memptr->slots[i] = 0;
        if(pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED) != 0)
            perror("pthread_mutexattr_setpshared");
        if(pthread_condattr_setpshared(&attrcond, PTHREAD_PROCESS_SHARED) != 0)
            perror("pthread_mutexattr_setpshared");
        pthread_mutex_init(&memptr->server[i], &attrmutex);
        pthread_cond_init(&memptr->serverCond[i], &attrcond);
    }
    

    while(1) {
        //lock thread, wait for client flag to be populated by client
        pthread_mutex_lock(&memptr->client);
        while(memptr->clientflag != FULL)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);

        //sem_wait
        sem_wait(queue);
        unsigned int num = memptr->number;

        for(int i = 0; i < NUM_SLOTS; i++) {
            if(memptr->serverflag[i] == EMPTY) {
                printf("slot found: %d\n", i);
                memptr->number = i;
                memptr->clientflag = EMPTY;
                pthread_cond_signal(&memptr->clientCond);
                pthread_mutex_unlock(&memptr->client);
                ServerQuery query = {num, i, memptr, queue};
                pthread_create(&queries[i], NULL, handleQuery, (void *) &query);
                break;
            }
        }
    }

    shmdt((void *) memptr);
    printf("server has detached it's shared memory...\n");
    exit(0);
}