# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"
# include "shell.h"
# include "timer.h"
# include "mm.h"

boot_info_t info;

void start_kernel(unsigned long hartid, unsigned long dtb_ptr){

    const void* fdt = (const void*)dtb_ptr;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("[Kernel] Error: Invalid DTB Magic Number.\n");

    // ================================== 找 UART address ======================================
    int uart_offset = fdt_path_offset(fdt, "/soc/serial");
    if(uart_offset >= 0){
        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, uart_offset, "reg", &len);

        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);
    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | reg1;
            if (reg0 == 0) detected_base = reg1;    

            UART_BASE = detected_base; 

            // 判斷 STRIDE
            if (UART_BASE == 0x10000000UL) {
                UART_STRIDE = 1;        // QEMU
            } else {
                UART_STRIDE = 4;        // OrangePi
            }

            uart_puts("[Kernel] DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");

        }else{
            uart_puts("[Kernel] Error: 'reg' property not found.\n");
        }
    }
    // ==========================================================================================


    // =================================== memory setup(Lab3) ===================================
    fdt_get_boot_info((const void*)dtb_ptr, &info);

    //print info
    uart_puts("[Kernel] Memory Start: "); uart_hex(info.mem_start); uart_puts("\n");
    uart_puts("[Kernel] Memory Size : "); uart_hex(info.mem_size); uart_puts("\n");
    uart_puts("[Kernel] Initrd Start : "); uart_hex(info.initrd_start); uart_puts("\n");
    uart_puts("[Kernel] Initrd End : "); uart_hex(info.initrd_end); uart_puts("\n");
    uart_puts("[Kernel] DTB Start : "); uart_hex(info.dtb_start); uart_puts("\n");
    uart_puts("[Kernel] DTB Size : "); uart_hex(info.dtb_size); uart_puts("\n");

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
    mm_free_lists();
    // ========================================================================================

    // find clock freq
    fdt_timer_init(fdt);

    uart_puts("Starting Kernel...\n");
    uart_puts("===== OSC LAB3 =====\n");
    kernel_shell();
}