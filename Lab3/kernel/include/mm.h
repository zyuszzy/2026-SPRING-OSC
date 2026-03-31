#ifndef MM_H
#define MM_H

#include "type.h"
#include "list.h"

#define PAGE_SIZE 4096
#define MAX_ORDER 7 
#define MEM_START 0x81000000
#define MEM_SIZE  0x01000000    // 16MB
#define FRAME_COUNT (MEM_SIZE / PAGE_SIZE)  // 4096 frames

struct frame{
    struct list_head list; 
    phy_addr_t addr;    // physical address
    int order;             
    int refcount;      // 0 = free
    int ischunk;
    int chunk_size;
};

void mm_init();
void *allocate(unsigned int size);
void free(void *ptr);

#endif