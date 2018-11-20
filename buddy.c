#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>

#define MIN 5
#define LEVELS 8
#define PAGE 4096

enum flag {Free, Taken};

struct head {
    enum flag status;
    short int level;
    struct head *next;
    struct head *prev;
};

struct head *flists[LEVELS] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL, NULL};


struct head *new() {
    struct head *new = (struct head *) mmap(NULL,
                                            PAGE, //max size
                                            PROT_READ | PROT_WRITE,
                                            MAP_PRIVATE | MAP_ANONYMOUS,
                                            -1, //fd? ignored in anonymous
                                            0 //offset
                                            );

    if (new == MAP_FAILED) {
        return NULL;
    }

    assert(((long int)new & 0xfff) == 0);
    new->status = Free;
    new->level = LEVELS -1; //levels are 0-7
    return new;
}

/*
 * Find the buddy of a block (by setting the proper bit for the level)
 * returns a block half size of the current "block" (as buddy)
 * Don't call buddy on highest level block
 */
struct head *buddy(struct head* block) {
    int level = block->level;
    long int mask = 0x1 << (level + MIN);
    return (struct head*)((long int) block ^ mask); //return 1 if bit we are interested in is 0, 0 otherwise
}

struct head *merge(struct head* block, struct head* buddy) {
    struct head *first;
    if (buddy < block) {
        first = buddy;
    } else {
        first = block;
    }
    first->level = first->level+1; //step up one level
    first->status = Free;
    return first;
}

void *withoutHead(struct head* address) {
    return (void*)(address + 1); //+1 seems to make address point to next free space
}

struct head *withHead(void *memory) {
    return ((struct head*) memory - 1);
}

//return level we need for a certain requested size
int level(int size) {
    int request = size + sizeof(struct head);

    int i = 0;
    request = request >> MIN;
    while (request > 0) {
        i++;
        request = request >> 1; //shift until the 1 "disappears", i is now the level
    }
    return i;
}

void test() {
    struct head* test = new();
    //struct head* test2 = new();
    printf("\n Test address: %p\n", test);
    //printf("\n Test2 address: %p\n", test2);

    //printf("%p\n", test+1);
    //printf("%d\n", sizeof(*test));

    //printf("Buddy: %p\n", buddy(test));
    test->level = test->level-1;
    struct head *bud= buddy(test);
    printf("\n Test address: %p\n", test);
    printf("Buddy: %p\n", bud);

    struct head *without = withoutHead(test);

    printf("Hide: %p\n", without);
    printf("Show: %p\n", withHead(without));

    printf("Merge: %p\n", merge(test, bud));
    printf("Merge: %p\n", merge(bud, test));
}

struct head *find(int level) {
    int i = level;
    printf("hej");
    while (flists[i] == NULL) {
        i++;
        if (i > LEVELS-1) {
            return NULL; //no free memory in list
        }
    }
    struct head *freeblock = flists[i]; //take first free block in list
    if (freeblock->next == NULL) {
        flists[i] = NULL;
    } else {
        flists[i] = freeblock->next; //checkfor or null?
    }

    while (i != level) {
        printf("%d", i);
        i--; //step down one level (we are splitting blocks)
        freeblock->status = Taken;
        freeblock->level = i;
        struct head *buddyToInsert = buddy(freeblock); //find buddy to be free
        buddyToInsert->status = Free;
        buddyToInsert->next = flists[i]; //buddy will be first in list on level i
        if (flists[i] != NULL) {
            flists[i]->prev = buddyToInsert; //previous first block will have buddy as previous
        }
        flists[i] = buddyToInsert; //buddy is first in list
    }
    return freeblock;
}

void *balloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    int lvl = level(size);
    //printf("%d", lvl);
    struct head *taken = find(lvl);
    return withoutHead(taken);
}

/*
   void bfree(void *memory) {
   if(memory != NULL) {
   struct head *block = withHead(memory);
   insert(block);
   }
   return;
   }
   */

int main() {
    printf("%d\n", level(30));

    struct head* start = new();
    flists[7] = start;

    printf("%p", flists[7]);

    if (flists[7] == NULL) {
        printf("jag är null");
    } else {
        printf("inte null");
    }

    printf("%d\n",flists[7]->level);
    printf("\nBlock: %p\n", start);
    //struct head* h = balloc(0);
    //printf("\nBlock: %p\n", h);
    size_t size = 2;
    struct head* test = balloc(size);
    printf("\nBlock: %p\n", test);

    for (int i=0;i<8;i++)
        printf("\n%p", flists[i]);

}
