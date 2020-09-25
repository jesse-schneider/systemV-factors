#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <string.h>


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


int main(int argc, char **argv) {

    unsigned int number = atol(argv[1]);
    int num_threads = 32;

    pthread_t threads[num_threads];
    
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

   return 0;
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