#include "memory.h"
void * processQuery(void *);


typedef struct Query {
    int slot;
    MemoryStruct *memptr;
} Query;

void main(int argc, char **argv) {

    //init shared memory variables
    key_t shmKey;
    int shmId;
    MemoryStruct *memptr;


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
        printf("\ncommand> ");
        gets(buffer);

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
            

        int slot = memptr->number;
        // printf("slot: %d\n", slot);
        Query q = {slot, memptr};
        pthread_mutex_unlock(&memptr->client);

        pthread_t queryThread;
        pthread_create(&queryThread, NULL, processQuery, (void *) &q);
        // pthread_join(queryThread, NULL);
    }
}


void * processQuery(void *query) {
    Query * q = (Query *) query;
    unsigned int slot = (*q).slot;
    MemoryStruct * memptr = (*q).memptr;
    int threads = 0;

    printf("slot: %d\n", slot);
    //while all threads are running
    while (threads < 32) {
        //lock on to allocated slot and wait for new data
        pthread_mutex_lock(&memptr->server[slot]);
        while (memptr->serverflag[slot] != FULL && memptr->serverflag[slot] != COMPLETE)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);

        //check the new data (either factor or completed thread)
        if(memptr->serverflag[slot] == FULL) {
            printf("factor: %d\r", memptr->slots[slot]);   
        } else if (memptr->serverflag[slot] == COMPLETE)
            threads++;

        //reset serverflag to empty and send signal to any blocked server threads
        memptr->serverflag[slot] = EMPTY;
        pthread_cond_signal(&memptr->serverCond[slot]);
        pthread_mutex_unlock(&memptr->server[slot]);
        usleep(3800); 
    }

    pthread_mutex_lock(&memptr->server[slot]);
    memptr->serverflag[slot] = 0;
    pthread_mutex_unlock(&memptr->server[slot]);

    printf("query complete");
    return NULL;
}