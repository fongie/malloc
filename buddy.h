#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

void test();
void test2();
void* balloc(size_t size);
void* bfree(void *memory);
void clearMemory();
