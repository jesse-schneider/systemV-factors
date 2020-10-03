#include "memory.h"
void * processQuery(void *);
void * displayProgress(void *);


typedef struct Progress {
    int * progress;
    int * queries;
} Progress;

typedef struct Query {
    int slot;
    int * progress;
    int * queries;
    MemoryStruct *memptr;
} Query;

int display = 0;

void main(int argc, char **argv) {

    //init shared memory variables
    key_t shmKey;
    int shmId;
    MemoryStruct *memptr;

    int progress[NUM_SLOTS] = {0};
    int queries = 0;


    //shared memory get function to get shared memory using unique key (made with ftok)
    shmKey = ftok(".", 'x');
    shmId = shmget(shmKey, sizeof(MemoryStruct), 0666);
    if(shmId < 0) {
        printf("*** shmget error (client) ***\n");
        perror("shmget");
        exit(1);
    }

    memptr = (MemoryStruct *) shmat(shmId, NULL, 0);
    if((int) memptr == -1) {
        printf("*** shamt error (client) ***\n");
        exit(1);
    }

    //client infinite loop
    while(1) {
        //94967285
        char buffer[100] = {0};
    
        //user prompt and input
        printf("\n32-bit integer> ");
        gets(buffer);
        printf("\n");

        if(queries >= 10) {
            printf("Warning: server currently too busy for your request.\n");
            continue;
        }
        
        //quit command
        if(strcmp(buffer, "quit") == 0) {
            printf("Exiting gracefully...\n");
            shmdt((void *) memptr);
            shmctl(shmId, IPC_RMID, NULL);
            exit(0);
        }

        //lock client mutex, put number into server, send signal to server
        pthread_mutex_lock(&memptr->client);
        memptr->number = atoi(buffer);
        memptr->clientflag = FULL;

        pthread_cond_signal(&memptr->clientCond);
        pthread_mutex_unlock(&memptr->client);
    
        usleep(500000);

        pthread_mutex_lock(&memptr->client);
        while (memptr->clientflag != EMPTY)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);
        
        if(memptr->clientflag == TOO_MANY) {
            printf("Warning: server is currently busy, please wait\n");
            continue;
        }
            

        int slot = memptr->number;
        Query q = {slot, progress, &queries, memptr};
        pthread_mutex_unlock(&memptr->client);

        pthread_t queryThread;
        pthread_create(&queryThread, NULL, processQuery, (void *) &q);
        queries++;
        // pthread_join(queryThread, NULL);
    }
}


void * processQuery(void *query) {
    pthread_t displayThread;
    Query * q = (Query *) query;
    int slot = (*q).slot;
    MemoryStruct * memptr = (*q).memptr;
    int * progress = (*q).progress;
    int * queries = (*q).queries;

    clock_t start, stop;
    double duration;
    start = clock();
    
    if(display == 0) {
        display = 1;
        Progress p = {progress, queries};
        pthread_create(&displayThread, NULL, displayProgress, (void *) &p);
    }
    

    //while all threads are running
    while (progress[slot] < NUM_THREADS) {
        //lock on to allocated slot and wait for new data
        pthread_mutex_lock(&memptr->server[slot]);
        while (memptr->serverflag[slot] != FULL && memptr->serverflag[slot] != COMPLETE)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);
        // printf("Progress: %d %\r", (progress[slot] * 100) / NUM_THREADS);

        //check the new data (either factor or completed thread)
        if(memptr->serverflag[slot] == FULL) {
            // printf("factor: %u\r", memptr->slots[slot]);
            fflush(stdout);
        } else if (memptr->serverflag[slot] == COMPLETE)
            progress[slot]++;

        //reset serverflag to empty and send signal to any blocked server threads
        memptr->serverflag[slot] = EMPTY;
        pthread_cond_signal(&memptr->serverCond[slot]);
        pthread_mutex_unlock(&memptr->server[slot]);
        //usleep(3800); 
        usleep(50000);
    }

    pthread_mutex_lock(&memptr->server[slot]);
    memptr->serverflag[slot] = 0;
    pthread_mutex_unlock(&memptr->server[slot]);
    progress[slot] = 0;
    (*queries)--;

    stop = clock();
    duration = (double) (stop-start)/CLOCKS_PER_SEC;
    printf("\n\nQuery %d time taken>>> %lf \n\n", (slot+1), duration);
    printf("\33[2K\r>");
    return NULL;
}


void * displayProgress(void * prog) {

    Progress * p = (Progress *) prog;
    int * progress = (*p).progress;
    int * queries = (*p).queries;

    while((*queries) > 0) {
        usleep(500000);
        printf("\33[2K\r");
        fflush(stdout);
        for(int i = 0; i < NUM_SLOTS; i++) {
            if(progress[i] != 0) {
                fflush(stdout);
                printf("Query %d: %d%\t", (i+1), (progress[i] * 100) / NUM_THREADS);
            }
        }
        printf(">");
        
    }
    display = 0;
    return NULL;   
}