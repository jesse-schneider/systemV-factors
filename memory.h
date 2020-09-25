#define EMPTY 0
#define FULL 1

struct MemoryStruct {
    char clientflag;
    unsigned int number;
    char serverflag[10];
    int slots[10];
};