# include "mm.h"
# include "uart.h"
# include "type.h"

unsigned long MEM_START;
unsigned long MEM_SIZE;
unsigned int  FRAME_COUNT;

// for early reserve
typedef struct{
    unsigned long start;
    unsigned long end;
} res_area_t;
res_area_t early_res[20]; 
int early_res_count = 0;

// for buddy system & slab alloc
struct frame *frames;
struct list_head free_areas[MAX_ORDER + 1];
int chunk_offsets[] = {16, 32, 48, 64, 128, 256, 512, 1024, 2048};
struct list_head chunk_pools[9];


// ================================================================================================================================

static void log_mm_range(const char *prefix, int index, int order){
    int page_count = (1 << order);
    uart_puts(prefix);
    uart_puts(" page ");
    uart_putd(index);
    uart_puts(" at order ");
    uart_putd(order);
    uart_puts(". Range of pages: [");
    uart_putd(index);
    uart_puts(", ");
    uart_putd(index + page_count - 1);
    uart_puts("]\n");
}


void memory_early_reserve(unsigned long start, unsigned long end){
    early_res[early_res_count].start = start;
    early_res[early_res_count].end = end;
    early_res_count++;
    /*uart_puts("[MM] Reserved range: ");
    uart_hex(start);
    uart_puts(" - ");
    uart_hex(end);
    uart_puts("\n");*/
}

void memory_reserve(unsigned long start, unsigned long size){
    if (frames == (void*)0) return;

    int start_index = (start - MEM_START)/PAGE_SIZE;
    int end_index = (start + size - 1 - MEM_START)/PAGE_SIZE;

    for(int i=start_index ; i<=end_index ; i++){
        if(i < 0 || i >= FRAME_COUNT)
            continue;
        frames[i].refcount = 1;
    }

    uart_puts("[MM] Reserved range: ");
    uart_hex(start);
    uart_puts(" - ");
    uart_hex(start + size);
    uart_puts("\n");
}

void mm_init(unsigned long mem_start, unsigned long mem_size){

    MEM_START = mem_start; 
    MEM_SIZE = mem_size;
    FRAME_COUNT = mem_size / PAGE_SIZE;
    unsigned long array_size = FRAME_COUNT * sizeof(struct frame);  //frame array size

    unsigned long candidate_addr = (unsigned long)_end;
    candidate_addr = (candidate_addr + 4095) & ~4095;

    int found = 0;
    while(!found){
        found = 1;       // Assume founded
        for(int i=0 ; i<early_res_count ; i++){
            if(!(candidate_addr + array_size <= early_res[i].start || candidate_addr >= early_res[i].end)){
                candidate_addr = (early_res[i].end + 4095) & ~4095;
                found = 0;
                break;
            }
        }
    }

    frames = (struct frame*)candidate_addr;

    // list init
    for(int i=0 ; i<=MAX_ORDER ; i++){
        INIT_LIST_HEAD(&free_areas[i]);
    }
    // chunk pool init
    for (int i=0; i<9; i++) {
        INIT_LIST_HEAD(&chunk_pools[i]);
    }

    // frame init
    for(int i=0 ; i<FRAME_COUNT ; i++) {
        frames[i].addr = MEM_START + (i * PAGE_SIZE);
        frames[i].refcount = 0;
        frames[i].order = -1;
        frames[i].ischunk = 0;
        frames[i].chunk_size = 0;
    }

    // reserve memory
    for(int i=0 ; i<early_res_count ; i++){
        memory_reserve(early_res[i].start, early_res[i].end - early_res[i].start);
    }
    memory_reserve((unsigned long)frames, array_size);
}

void mm_final_init(){
    // slice into MAX_ORDER
    for(int i=0 ; i<FRAME_COUNT ; i+=(1 << MAX_ORDER)){
        
        int have_block_reserved = 0;
        for(int j=0 ; j<(1 << MAX_ORDER) ; j++){
            if((i + j) < FRAME_COUNT && frames[i + j].refcount == 1){
                have_block_reserved = 1;
                break;
            }
        }
        
        if(!have_block_reserved){       // inside not have reserved block
            frames[i].order = MAX_ORDER;
            list_add_tail(&frames[i].list, &free_areas[MAX_ORDER]);
        }
    }
}

void mm_free_lists(){
    uart_puts("\n-------- Current Buddy System Status --------\n");
    for(int i=0 ; i<=MAX_ORDER ; i++){
        int count = 0;
        struct list_head *curr;

        list_for_each(curr, &free_areas[i]){
            count++;
        }
        
        uart_puts("Order ");
        uart_putd(i);
        uart_puts(": ");
        uart_putd(count);
        uart_puts(" blocks (");
        uart_putd(count * (1 << i) * PAGE_SIZE / 1024);     //KB
        uart_puts(" KB)\n");
    }
    uart_puts("-----------------------------------------------\n\n");
}

// size=bytes
void* allocate(unsigned int size){

    // -------------------------------------- slab alloc -----------------------------------
    if(size < 4096){
        int pool_index = -1;

        // find suitable chunk
        for(int i=0 ; i<9 ; i++){
            if(size <= chunk_offsets[i]){
                pool_index = i;
                break;
            }
        }

        if(!list_empty(&chunk_pools[pool_index])){
            struct list_head *c = chunk_pools[pool_index].next;
            list_del(c);
            uart_puts("  [Chunk] Allocate 0x");
            uart_hex((phy_addr_t)c);
            uart_puts(" from existing pool, chunk size ");
            uart_putd(chunk_offsets[pool_index]);
            uart_puts(".\n");
            return (void *)c;
        }

        // chunk pool not enough, ask from frame alloc
        char *new_frame = (char *)allocate(4096);
        if(!new_frame)
            return (void *)0;
        
        int frame_index = ((phy_addr_t)new_frame - MEM_START)/PAGE_SIZE;
        frames[frame_index].ischunk = 1; 
        frames[frame_index].chunk_size = chunk_offsets[pool_index];
        
        // split frame to chunk size
        int c_size = chunk_offsets[pool_index];
        for(int i=0 ; i<PAGE_SIZE ; i+=c_size){
            if(i == 0) continue;
            struct list_head *chunk = (struct list_head *)(new_frame + i);
            list_add_tail(chunk, &chunk_pools[pool_index]);
        } 

        uart_puts("  [Chunk] Allocate 0x");
        uart_hex((phy_addr_t)new_frame);
        uart_puts(" from New Page(sliced), chunk size ");
        uart_putd(chunk_offsets[pool_index]);
        uart_puts(".\n");
        return (void *)new_frame;
    }



    // ------------------------------------- frame alloc ------------------------------------
    // need how much page
    int target_order = 0;
    unsigned int need_page_num = (size + (PAGE_SIZE - 1))/PAGE_SIZE;
    while((1 << target_order) < need_page_num)
        target_order++;

    if(target_order > MAX_ORDER)
        return (void *)0;

    for(int current_order = target_order ; current_order <= MAX_ORDER ; current_order++){
        if(!list_empty(&free_areas[current_order])){
            struct list_head *current_list_head = free_areas[current_order].next;
            
            list_del(current_list_head);
            struct frame *current_frame = list_entry(current_list_head, struct frame, list);
            int current_index = current_frame - frames;       // (current_frame -frames)=PFN
            log_mm_range("  [-] Remove", current_index, current_order);


            // split
            while(current_order > target_order){
                current_order--;
                int buddy_index = current_index + (1 << current_order);
                frames[buddy_index].order = current_order;
                
                list_add(&frames[buddy_index].list, &free_areas[current_order]);    // buddy ?[?Jfree ares
                log_mm_range("  [+] Add", buddy_index, current_order);
            }

            current_frame->order = target_order;
            current_frame->refcount = 1; 

            // print out final reuslt
            uart_puts("  [Page] Allocate 0x");
            uart_hex(current_frame->addr);
            uart_puts(" (page ");
            uart_putd((current_frame->addr - MEM_START)/PAGE_SIZE);
            uart_puts(") at order ");
            uart_putd(target_order);
            uart_puts(".\n");


            return (void *)current_frame->addr;     
        }
    }

    return (void *)0;
}

void free(void *ptr){
    if(!ptr)    return;

    phy_addr_t addr = (phy_addr_t)ptr;
    int frame_index = (addr - MEM_START)/PAGE_SIZE;
    struct frame *f = &frames[frame_index];

    // ---------------------------------------- slab ----------------------------------------
    if(f->ischunk){
        struct list_head *chunk = (struct list_head *)ptr;

        int pool_index = -1;
        for(int i=0 ; i<9 ; i++){
            if(f->chunk_size == chunk_offsets[i]){
                pool_index = i;
                break;
            }
        }
        list_add(chunk, &chunk_pools[pool_index]);

        uart_puts("  [Chunk] Free 0x");
        uart_hex(addr);
        uart_puts(" at chunck size ");
        uart_putd(f->chunk_size);
        uart_puts(".\n");
        return;
    }

    // ------------------------------------- frmae ------------------------------------------
    int current_order = f->order;
    f->refcount = 0;

    // ©¹?W§ä?i¤£?i?H?X¨Ö
    while(current_order < MAX_ORDER){
        int buddy_index = frame_index ^ (1 << current_order);
        struct frame *buddy = &frames[buddy_index];

        if(buddy->ischunk || buddy->refcount == 1 || buddy->order != current_order)
            break;

        uart_puts("  [*] Buddy found! buddy idx: ");
        uart_putd(buddy_index);
        uart_puts(" for page ");
        uart_putd(frame_index);
        uart_puts(" with order ");
        uart_putd(current_order);
        uart_puts("\n");

        list_del(&buddy->list);
        log_mm_range("  [-] Remove", buddy_index, current_order);

        if(buddy_index < frame_index){
            frame_index = buddy_index;
            f = buddy;
        }

        current_order++;
        f->order = current_order;
    }

    list_add(&f->list, &free_areas[current_order]);
    log_mm_range("  [+] Add", frame_index, current_order);

    // print out final result
    uart_puts("  [Page] Free 0x");
    uart_hex(addr);
    uart_puts(" (page ");
    uart_putd((addr - MEM_START)/PAGE_SIZE);
    uart_puts(") , add back to order ");
    uart_putd(current_order);
    uart_puts(".\n");
}
