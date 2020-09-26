#include <sys/types.h>


struct semaphore {
    int count;
    queueType queue;
}

void semWait(semaphore s) {
    s.count--;
    if(s.count < 0) {
        
    }

}