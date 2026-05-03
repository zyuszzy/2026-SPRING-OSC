# include "uart.h"
# include "type.h"
# include "string.h"
# include "thread.h"
# include "fdt.h"
# include "shell.h"
# include "timer.h"
# include "plic.h"
# include "mm.h"

extern void idle();
extern void foo();
extern void video_init();

boot_info_t info;
unsigned long boot_cpu_hartid;

void start_kernel(unsigned long hartid, unsigned long dtb_ptr){

    const void* fdt = (const void*)dtb_ptr;
    boot_cpu_hartid = hartid;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("[Kernel] Error: Invalid DTB Magic Number.\n");

    fdt_uart_init(fdt);
    fdt_timer_init(fdt); // find clock freq
    fdt_plic_init(fdt);   // find PLIC_BASE

    // =================================== memory setup(Lab3) ===================================
    fdt_get_boot_info((const void*)dtb_ptr, &info);

    // early reserve(record reserve mem)
    memory_early_reserve((unsigned long)_start, (unsigned long)_end);   //kernel
    memory_early_reserve(info.dtb_start, info.dtb_start + info.dtb_size);     // dtb
    memory_early_reserve(info.initrd_start, info.initrd_end);   // initrd
    fdt_additional_reserve_mem((const void*)dtb_ptr);       // additional reserved mem

    // init & startup alloc & reserve memory
    mm_init(info.mem_start, info.mem_size);

    // slice usable memory into MAX_ORDER
    mm_final_init(); 
    // ========================================================================================

    
    uart_init();
    plic_init();
    video_init();
    timer_init();

    unsigned long sie_mask = (1 << 9) | (1 << 5);       // (1 <<9):sie.SEIE (External interrupt)  //(1 << 5):// sie.STIE(open accept timer inerrupt)
    asm volatile("csrs sie, %0" : : "r"(sie_mask));
    asm volatile("csrsi sstatus, 1 << 1");      // ssatatus.SIE (Global inerrupt)

    // Basic 1
    /*struct task_struct* init_task = thread_create(idle);
    asm volatile("mv tp, %0" : : "r"(init_task));
    for (int i = 0; i < 3; i++) {
        thread_create(foo);
    }
    idle();*/

    struct task_struct* init_task = thread_create(idle);
    asm volatile("mv tp, %0" : : "r"(init_task));
    thread_create(shell_thread);

    while(1);
}