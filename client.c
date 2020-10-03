#include "memory.h"

/** Progress Struct Represents data to be sent to the progress display thread **/
typedef struct Progress {
    int * progress;
    int * queries;
} Progress;



/** Queruy Struct Represents data to be sent to a query thread **/
typedef struct Query {
    int slot;
    int * progress;
    int * queries;
    MemoryStruct *memptr;
} Query;


//global variable: if == 0, no queries are printing, if == 1, another thread is already printing 
int display = 0;

void main(int argc, char **argv) {
    //init shared memory variables
    key_t shmKey;
    int shmId;
    MemoryStruct *memptr;

    //progress reporting variables
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
        fgets(buffer, 100, stdin);
        printf("\n");

        //show user warning message if server is busy
        if(queries >= 10) {
            printf("Warning: server currently too busy for your request.\n");
            continue;
        }
        
        //quit command
        if(strcmp(buffer, "quit\n") == 0) {
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
    
        usleep(5000);

        //get slot number assigned by server, send query to it's own thread to await and process results
        pthread_mutex_lock(&memptr->client);
        while (memptr->clientflag != EMPTY)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);

        int slot = memptr->number;
        Query q = {slot, progress, &queries, memptr};
        pthread_mutex_unlock(&memptr->client);

        pthread_t queryThread;
        pthread_create(&queryThread, NULL, processQuery, (void *) &q);
        queries++;
    }
}



/** Function to process the results of a single query in it's own thread **/
void * processQuery(void *query) {
    //extract data from the Query struct
    Query * q = (Query *) query;
    int slot = (*q).slot;
    MemoryStruct * memptr = (*q).memptr;
    int * progress = (*q).progress;
    int * queries = (*q).queries;

    //init clock and start timer
    clock_t start, stop;
    double duration;
    start = clock();

    //if not already printing progress, start printing them
    if(display == 0) {
        display = 1;
        pthread_t displayThread;
        Progress p = {progress, queries};
        pthread_create(&displayThread, NULL, displayProgress, (void *) &p);
    }
    

    //while all threads are running
    while (progress[slot] < NUM_THREADS) {
        //lock on to allocated slot and wait for new data
        pthread_mutex_lock(&memptr->server[slot]);
        while (memptr->serverflag[slot] != FULL && memptr->serverflag[slot] != COMPLETE)
            pthread_cond_wait(&memptr->clientCond, &memptr->client);

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
        usleep(3800); 
        //usleep(50000);
    }

    //reset all variables for use by a future query
    pthread_mutex_lock(&memptr->server[slot]);
    memptr->serverflag[slot] = 0;
    pthread_mutex_unlock(&memptr->server[slot]);
    progress[slot] = 0;
    (*queries)--;

    //stop and show the cpu time taken to perform the request 
    stop = clock();
    duration = (double) (stop-start)/CLOCKS_PER_SEC;
    printf("\n\nQuery %d time taken>>> %lf \n\n", (slot+1), duration);
    printf("\33[2K\r>");
    return NULL;
}



/** Function to display all current queries progress, refreshing every 500 milliseconds **/
void * displayProgress(void * prog) {

    //extract data from Progress struct
    Progress * p = (Progress *) prog;
    int * progress = (*p).progress;
    int * queries = (*p).queries;

    //while queries exist, print out progress of every query
    while((*queries) > 0) {
        usleep(500000);
        printf("\33[2K\r");
        for(int i = 0; i < NUM_SLOTS; i++) {
            if(progress[i] != 0) {
                printf("Query %d: %d%%\t", (i+1), (progress[i] * 100) / NUM_THREADS);
            }
        }
        printf(">");
        
    }
    display = 0;
    return NULL;   
}