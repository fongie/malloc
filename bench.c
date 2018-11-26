#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "buddy.h"

#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>

#define BALLOC_SIZE 5

double time_balloc(int size) {
    clock_t c_start, c_stop;

    c_start = clock();

    void *mem = balloc(size);

    c_stop = clock();

    bfree(mem);

    double proc = ((double)(c_stop - c_start))/CLOCKS_PER_SEC;
    return proc;
}
double time_free(int size) {
    clock_t c_start, c_stop;

    void* bal = balloc(size);
    c_start = clock();

    bfree(bal);

    c_stop = clock();

    double proc = ((double)(c_stop - c_start))/CLOCKS_PER_SEC;
    return proc;
}

double bench_balloc(int minsize, int maxsize, int requestedblocks, int allocatedblocks) {
    clearMemory();
    int size;
    //allocation
    for (int i = 0; i < allocatedblocks; i++) {
        size = (rand() % (maxsize-minsize + 1)) + minsize;
        balloc(size);
    }
    double res;
    for (int i = 0; i < 1000; i++) {
        size = (rand() % (maxsize-minsize + 1)) + minsize;
        res += time_balloc(size);
    }
    return res/1000;
}
double bench_free(int minsize, int maxsize, int requestedblocks, int allocatedblocks) {
    clearMemory();
    int size;
    //allocation
    for (int i = 0; i < allocatedblocks; i++) {
        size = (rand() % (maxsize-minsize + 1)) + minsize;
        balloc(size);
    }
    double res;
    for (int i = 0; i < 1000; i++) {
        size = (rand() % (maxsize-minsize + 1)) + minsize;
        res += time_free(size);
    }
    return res/1000;
}

void bench(int minsize, int maxsize, int requestedblocks) {
    srand(time(NULL));

    printf("Cost of a bfree and balloc:\n");
    double res = bench_balloc(minsize,maxsize,requestedblocks,0);
    printf("Average for 1 balloc is:\t %.15f\n", res);

    res = bench_free(minsize,maxsize,requestedblocks,0);
    printf("Average for 1 free is:\t\t %.15f\n", res);

    printf("With 25 percent allocation:\n");

    res = bench_balloc(minsize,maxsize,requestedblocks,requestedblocks/4);
    printf("Average for 1 balloc\n on 25 percent allocation is:\t %.15f\n", res/1000);

    res = bench_free(minsize,maxsize,requestedblocks,requestedblocks/4);
    printf("Average for 1 free on\n 25 percent allocation is:\t %.15f\n", res/1000);

    printf("With 75 percent allocation:\n");

    res = bench_balloc(minsize,maxsize,requestedblocks,(requestedblocks/4)*3);
    printf("Average for 1 balloc\n on 75 percent allocation is:\t %.15f\n", res/1000);

    res = bench_free(minsize,maxsize,requestedblocks,(requestedblocks/4)*3);
    printf("Average for 1 free on\n 75 percent allocation is:\t %.15f\n", res/1000);
    //why longer on empty allocation?
    //because balloc have to split all the way down to lvl ? from lvl 7
    //and bfree has to merge all way from lvl ? to lvl7 (or empty or buddy)

    //if blocks are already allocated, they might find a free block right away in list, or if freeing, could "stop" the chain of operations earlier aswell
    //
    //Note on our benchmark program:
    //Since we are always freeing the same block we just allocated and vice versa, we never have to traverse the linked lists in the benched program. A better program would be one that randomly takes allocated blocks and frees them, for example from a list previously populated. Then, the freeing operation (?allocating too?) would take longer than in our program because it has to traverse the double linked lists.
}

int main() {
    bench(2,4096,100000);
}
