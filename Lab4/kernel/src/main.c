# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"
# include "shell.h"
# include "timer.h"
# include "plic.h"
# include "mm.h"

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

    // =================================== memory setup(Lab3) ===================================
    fdt_get_boot_info((const void*)dtb_ptr, &info);

    // early reserve(record reserve mem)
    memory_early_reserve((unsigned long)_start, (unsigned long)_end);   //kernel
    memory_early_reserve(info.dtb_start, info.dtb_start + info.dtb_size);     // dtb
    memory_early_reserve(info.initrd_start, info.initrd_end);   // initrd
    fdt_additional_reserve_mem((const void*)dtb_ptr);       // additional reserved mem

    // init & startup alloc & reserve memory
    uart_puts("[Kernel] Reserving Memory...\n");
    mm_init(info.mem_start, info.mem_size);
    uart_puts("[Kernel] Memory Reserved successful!\n");

    // slice usable memory into MAX_ORDER
    mm_final_init(); 

    // print num for all orders of frame
    // mm_free_lists();
    // ========================================================================================

    // find clock freq
    fdt_timer_init(fdt);
    timer_init();

    // find PLIC_BASE
    fdt_plic_init(fdt);
    plic_init();

    uart_init();

    asm volatile("csrs sie, %0" : : "r"(1 << 9));   // sie.SEIE (External interrupt)
    asm volatile("csrsi sstatus, 1 << 1");      // ssatatus.SIE (Global inerrupt)
    kernel_shell();
}