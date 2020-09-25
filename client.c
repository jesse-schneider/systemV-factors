#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "memory.h"


/*** clear shared memory: ipcs and ipcrm ***/

void main(int argc, char **argv) {
    key_t shmKey;
    int shmId;
    struct MemoryStruct *memptr;


    //shared memory get function to get shared memory using unique key (made with ftok)
    shmKey = ftok(".", 'x');
    shmId = shmget(shmKey, sizeof(struct MemoryStruct), IPC_CREAT | 0666);
    printf("mem id: %d\n", shmId);
    if(shmId < 0) {
        printf("*** shmget error (client) ***\n");
        perror("shmget");
        exit(1);
    }


    memptr = (struct MemoryStruct *) shmat(shmId, NULL, 0);
    if((int) memptr == -1) {
        printf("*** shamt error (client) ***\n");
        exit(1);
    }

    printf("client has attached the shared memory...\n");
    memptr->serverflag;


    memptr->clientflag = EMPTY;
    //client infinite loop
    while(1) {
        char buffer[100] = {0};
     
        //user prompt and input
        printf("command> ");
        gets(buffer);

        //quit command
        if(strcmp(buffer, "quit") == 0) {
            printf("Exiting gracefully...\n");
            shmdt((void *) memptr);
            shmctl(shmId, IPC_RMID, NULL);
            exit(0);
        }

        memptr->number = atol(buffer);
        printf("client has filled  %u to shared memory...\n" ,memptr->number);
        memptr->clientflag = FULL;

        while (memptr->clientflag != EMPTY) {
            sleep(1);
        }

        int slot = memptr->number;
        printf("slot: %d\n", slot);
    }
}