#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>


#define MIN  5     
#define LEVELS 8   
#define PAGE 4096

enum flag {Free = 0, Taken = 1};


struct head {
  enum flag status;
  short int level;
  struct head *next;
  struct head *prev;  
};


/* The free lists */

struct head *flists[LEVELS] = {NULL};


/* These are the low level bit fidling operations */
  
struct head *new() {
  struct head *new = mmap(NULL, PAGE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if(new == MAP_FAILED) {
    printf("mmap failed: error %d\n", errno);
    return NULL;
  }
  assert(((long int)new & 0xfff) == 0);  // 12 last bits should be zero 
  new->status = Free;
  new->level = LEVELS -1;
  return new;
}


struct head *buddy(struct head* block) {
  int index = block->level;
  long int mask =  0x1 << (index + MIN);
  return (struct head*)((long int)block ^ mask);
}

/*
 *does not work
struct head *primary(struct head* block) {
  int index = block->level;
  long int mask =  0xffffffffffffffff << (1 + index + MIN);
  return (struct head*)((long int)block & mask);
}
*/
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

struct head *split(struct head *block) {
  int index = block->level - 1;
  int mask =  0x1 << (index + MIN);
  return (struct head *)((long int)block | mask );
}


void *hide(struct head* block) {
    if (block == 0x0)
        return NULL;
    return (void*)(block + 1);
}

struct head *magic(void *memory) {
    return (struct head*)(((struct head*)memory) - 1);
}

int level(int req) {
    int total = req  + sizeof(struct head);

    int i = 0;
    int size = 1 << MIN;
    while(total > size) {
        size <<= 1;
        i += 1;
    }
    return i;
}


struct head *find(int level) {
    int i = level;
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
        i--; //step down one level (we are splitting blocks)
        //freeblock->status = Taken;
        freeblock->level = i;
        struct head *buddyToInsert = buddy(freeblock); //find buddy to be free
        buddyToInsert->status = Free;
        buddyToInsert->level = i;
        buddyToInsert->next = flists[i]; //buddy will be first in list on level i
        if (flists[i] != NULL) {
            flists[i]->prev = buddyToInsert; //previous first block will have buddy as previous
        }
        flists[i] = buddyToInsert; //buddy is first in list
    }
    freeblock->status = Taken;
    freeblock->next = NULL;
    freeblock->prev = NULL;
    return freeblock;
}



void *balloc(size_t size) {
    if( size == 0 ){
        return NULL;
    }
    int index = level(size);
    struct head *taken = find(index);
    return hide(taken);
}

void removeFromList(struct head *block) {
    int level = block->level;
    //printf("Level: %d\n", level);

    //if we are first in our linked list
    if (block == flists[level]) {
        //if we are only block in linked list
        if (block->next == NULL) {
            flists[level] = NULL;
            return;
        }
        flists[level] = block->next;
        return;
    }

    //find block in linked list
    struct head *currentBlock = flists[level];
    while (!(&currentBlock == &block)) {
        currentBlock = currentBlock->next;
    }

    //remove block from linked list
    struct head *prev = block->prev;
    if (block->next == NULL) {
        prev->next = NULL;
    } else {
        struct head *next = block->next;
        prev->next = next;
    }
}

void insert(struct head *block) {
    // for you to implement
    // vilken level är du på?
    int level = block->level;
    printf("INSERT on level: %d\n", level);

    //har du nån buddy som är fri?
    struct head* bud = buddy(block);

    printf("\nBlock: %p status %d\n", block, block->status);
    if (level == LEVELS-1) {
        flists[LEVELS-1] = block;
        return;
    }
    printf("Buddy: %p status %d level: %d\n", bud, bud->status, bud->level);

    if ((*bud).status == Free) {
        //då lägg ihop dom och gör samma sak på nivån över
        struct head *merged = merge(bud, block);
        merged->level = level + 1;
        removeFromList(bud);
        insert(merged);
        return;
    } else {
        //annars: placera in den i frilistan
        //printf("IM NOT FREE\n");
        block->next = flists[level];
        flists[level] = block;
        return;
    }
}

void bfree(void *memory) {

    if(memory != NULL) {
        struct head *block = magic(memory);
        insert(block);
    }
    return;
}


// Test sequences

void test() {

    printf("size of head is: %ld\n", sizeof(struct head));
    printf("level for  7 should be 0 : %d\n", level(7));
    printf("level for  8 should be 0 : %d\n", level(8));
    printf("level for  9 should be 1 : %d\n", level(9));  
    printf("level for  20 should be 1 : %d\n", level(20));  
    printf("level for  40 should be 1 : %d\n", level(40));
    printf("level for  41 should be 2 : %d\n", level(41));    

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

    struct head *without = hide(test);

    printf("Hide: %p\n", without);
    printf("Show: %p\n", magic(without));

    printf("Merge: %p\n", merge(test,bud));
    printf("Merge: %p\n", merge(bud,test));

    //printf("%d\n", level(30));

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
    struct head* test2 = balloc(size);
    printf("\nBlock: %p\n", test2);
    printf("\nTest2 flag: %d\n", magic(test2)->status);

    printf("\nEnum free: %d, Taken: %d\n", Free, Taken);
    for (int i=0;i<8;i++)
        printf("\nLevel %d: %p", i,flists[i]);

    struct head* test3 = balloc(size);
    printf("\nTest 3 ska vara tagen: %d\n", magic(test3)->status);
    printf("\nTest 3 ska vara tagen: %d\n", magic(test3)->status == Taken);
    struct head* budtest3 = buddy(magic(test3));
    printf("\nBuddy till Test 3 ska vara free: %d\n", budtest3->status);
    printf("\nBuddy till buddyTest 3 ska vara tagen: %d\n", buddy(budtest3)->status);
    printf("\nBuddy till level 6 ska vara tagen: %d\n", buddy(flists[5])->status);
    printf("\nTest3 ska vara samma som buddy(test2):\nTest3:%p\nTest2:%p\n", test3, buddy(test2));

    struct head* test4 = balloc(41);
    printf("\nFreelist på nivå två kommer vara tom:\n");
    for (int i=0;i<8;i++)
        printf("\nLevel %d: %p", i,flists[i]);

    printf("\nFreelist på nivå två kommer inte vara tom, men nivå 3 kommer vara tom:\n");
    struct head* test5 = balloc(41);
    for (int i=0;i<8;i++)
        printf("\nLevel %d: %p", i,flists[i]);

    struct head* nomemory = balloc(4095);
    printf("\nShouldnt be able to find free memory: %p\n", nomemory);

    struct head* flag = new();
    printf("\nFlag: %d", flag->status);
    flag->status = Taken;
    printf("\nFlag: %d\n", flag->status);

    bfree(test2);
    bfree(test3);
    //bfree(test4);

}

void test2() {
    struct head* start = new();
    flists[7] = start;

    struct head* test = balloc(41);
    for (int i=0;i<8;i++)
        printf("\nLevel %d: %p", i,flists[i]);

    bfree(test);
    for (int i=0;i<8;i++)
        printf("\nLevel %d: %p", i,flists[i]);

    printf("Dessa borde vara samma: %p %p\n", start, flists[7]);
}

/*
int main() {
    //test();
    test2();
    return 0;
}
*/
