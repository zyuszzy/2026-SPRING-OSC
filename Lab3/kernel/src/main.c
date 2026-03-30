# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"
# include "shell.h"


void start_kernel(unsigned long hartid, unsigned long dtb_ptr){

    const void* fdt = (const void*)dtb_ptr;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("[Kernel] Error: Invalid DTB Magic Number.\n");


    // ================================== 找 UART address =================================
    int uart_offset = fdt_path_offset(fdt, "/soc/serial");
    // 找到
    if(uart_offset >= 0){
        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, uart_offset, "reg", &len);

        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);
    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | reg1;
            if (reg0 == 0) detected_base = reg1;    // 處理常見的 32-bit 位址

            UART_BASE = detected_base; 

            // 判斷 STRIDE
            if (UART_BASE == 0x10000000UL) {
                UART_STRIDE = 1;        // QEMU
            } else {
                UART_STRIDE = 4;        // OrangePi
            }

            uart_puts(">>> DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");

        }else{
            uart_puts("[Kernel] Error: 'reg' property not found.\n");
        }
    }
    // ====================================================================================

    uart_puts("Starting Kernel...\n");
    uart_puts("===== OSC LAB3 =====\n");

    kernel_shell();
}