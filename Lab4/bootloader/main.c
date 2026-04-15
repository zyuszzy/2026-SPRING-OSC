#ifdef USE_QEMU
    #define KERNEL_LOAD_ADDR 0x82000000UL
#else
    #define KERNEL_LOAD_ADDR 0x20000000UL
#endif

# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"


// receive kernel through UART and load kernel
void load_kernel_uart(unsigned long hartid, unsigned long dtb_ptr){
    char* kernel_address = (char*)KERNEL_LOAD_ADDR;    // UART
    unsigned int magic = 0;
    unsigned int kernel_size = 0;

    uart_puts("Please run the Python script for loading kernel...\n");

    // check magic
    for(int i=0 ; i <4 ; i++)
        ((char*)&magic)[i] = uart_getc_raw();
    if(magic != 0x544F4F42){
        uart_puts("Error: magic number is not match.\n");
        return;
    }

    // read kernel size
    for(int i=0 ; i<4 ; i++)
        ((char*)&kernel_size)[i] = uart_getc_raw();
    uart_puts("Loading kernel, the Kernel Size: ");
    uart_hex(kernel_size);
    uart_puts("\n");

    // 開始讀kernel
    for(int i=0 ; i<kernel_size ; i++)
        kernel_address[i] = uart_getc_raw();
    uart_puts("Kernel loaded successfully! Jump to kernel ");
    if(KERNEL_LOAD_ADDR == 0x82000000UL)
        uart_puts("0x82000000 ...\n");
    else if(KERNEL_LOAD_ADDR == 0x20000000UL)
        uart_puts("0x20000000 ...\n");

    // function pointer
    void (*kernel_entry)(unsigned long, unsigned long) = (void (*)(unsigned long, unsigned long))KERNEL_LOAD_ADDR;
    kernel_entry(hartid, dtb_ptr);     // jump to kernel
}

// receive user's input
void shell(unsigned long hartid, unsigned long dtb_ptr){
    char input_buffer[64];
    int input_index = 0;

    uart_puts("\nStarting Bootloader Shell ...\n");
    uart_puts("===== OSC LAB3 =====\n");

    while(1){
        uart_puts("bootloader > ");
        input_index = 0;

        // 讀整行指令
        while(1){
            char input_c = uart_getc();
            if(input_c == '\b' || input_c == 127){
                if(input_index > 0){
                    input_index--;
                    uart_puts("\b \b"); 
                }
                continue;
            }

            uart_putc(input_c);

            if(input_c == '\n'){
                input_buffer[input_index] = '\0';
                break;
            }else if(input_index < 63){
                input_buffer[input_index++] = input_c;
            }
        }

        // 空白行
        if(input_index == 0) continue;
        
        if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  help  - show all commands.\n");
            uart_puts("  load  - receive the kernel image over UART.\n");
        }else if(strcmp(input_buffer, "load") == 0){
            load_kernel_uart(hartid, dtb_ptr);
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}


void main(unsigned long hartid, unsigned long dtb_ptr){      // a0:hartid a1:dtb_ptr
    
    const void* fdt = (const void*)dtb_ptr;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("Error: Invalid DTB Magic Number.\n");

    int offset = fdt_path_offset(fdt, "/soc/serial");
    // 找到
    if(offset >= 0){

        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, offset, "reg", &len);

        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);

    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | (unsigned long)reg1;
            if (reg0 == 0) detected_base = reg1;    // 處理常見的 32-bit 位址


            UART_BASE = detected_base;
            __asm__ volatile ("fence rw, rw"); // 確保之前的寫入先完成，後續讀取再開始

            uart_puts(">>> DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");
        }else{
            uart_puts("Error: 'reg' property not found.\n");
        }
    }else{
        uart_puts("Error: UART Node NOT found in DTB\n");
    }

    shell(hartid, dtb_ptr);

}