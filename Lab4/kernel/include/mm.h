#ifndef MM_H
#define MM_H

#include "type.h"
#include "list.h"

#define PAGE_SIZE 4096
#define MAX_ORDER 7 
extern unsigned long MEM_START;
extern unsigned long MEM_SIZE;
extern unsigned int  FRAME_COUNT;
extern char _start[], _end[];

struct frame{
    struct list_head list; 
    phy_addr_t addr;    // physical address
    int order;             
    int refcount;      // 0 = free
    int ischunk;
    int chunk_size;
};

void mm_init(unsigned long mem_start, unsigned long mem_size);
void *allocate(unsigned int size);
void free(void *ptr);
void memory_early_reserve(unsigned long start, unsigned long end);
void mm_final_init();
void mm_free_lists();

#endif