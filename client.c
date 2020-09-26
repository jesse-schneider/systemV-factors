#include "memory.h"

void * processQuery(void *);


typedef struct Query {
    int slot;
    MemoryStruct *memptr;
} Query;

/*** clear shared memory: ipcs and ipcrm ***/

void main(int argc, char **argv) {
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
    // while(1) {
        char buffer[100] = "4294967285";
     
        //user prompt and input
        printf("command> ");
        // gets(buffer);

        //quit command
        if(strcmp(buffer, "quit") == 0) {
            printf("Exiting gracefully...\n");
            shmdt((void *) memptr);
            shmctl(shmId, IPC_RMID, NULL);
            exit(0);
        }

        memptr->number = atoi(buffer);
        printf("client has filled %u to shared memory...\n" ,memptr->number);

        pthread_mutex_unlock(&memptr->client);
        memptr->clientflag = FULL;
        

        while (memptr->clientflag != EMPTY)
            ;
        int slot = memptr->number;
        printf("slot: %d\n", slot);
        Query q = {slot, memptr};

        pthread_t queryThread;
        pthread_create(&queryThread, NULL, processQuery, (void *) &q);
        pthread_join(queryThread, NULL);
    // }
}


void * processQuery(void *query) {

    Query * q = (Query *) query;
    unsigned int slot = (*q).slot;
    MemoryStruct * memptr = (*q).memptr;
    int threads = 0;

    while (threads < 31) {
        while (memptr->serverflag[slot] == EMPTY)
                ;

            if(memptr->serverflag[slot] == FULL) {
                printf("factor: %d\n", memptr->slots[slot]);
                // lock serverflag with mutex
                pthread_mutex_unlock(&memptr->server[slot]);
                memptr->serverflag[slot] = EMPTY;
                
            } else if (memptr->serverflag[slot] == COMPLETE) {
                threads++;
                printf("Threads: %d\n", threads);
                // lock serverflag with mutex
                pthread_mutex_unlock(&memptr->server[slot]);
                memptr->serverflag[slot] = EMPTY;
            }
            
            
            // unlock serverflag with mutex
                 
    }

    printf("query complete");

    
    return NULL;
}