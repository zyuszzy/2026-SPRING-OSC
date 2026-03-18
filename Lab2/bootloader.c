#ifdef USE_QEMU
    #define KERNEL_LOAD_ADDR 0x82000000UL
#else
    #define KERNEL_LOAD_ADDR 0x20000000UL
#endif

#include "uart.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// receive kernel through UART and load kernel
void load_kernel_uart(){
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
    uart_puts("Kernel loaded successfully! Jump to kernel");
    if(KERNEL_LOAD_ADDR == 0x82000000UL)
        uart_puts("0x82000000 ...\n");
    else if(KERNEL_LOAD_ADDR == 0x20000000UL)
        uart_puts("0x20000000 ...\n");

    // function pointer
    void (*kernel_entry)(void) = (void (*)(void))KERNEL_LOAD_ADDR;
    kernel_entry();     // jump to kernel
}

// receive user's input
void shell(){
    char input_buffer[64];
    int input_index = 0;

    uart_puts("\nStarting Bootloader Shell ...\n");
    uart_puts("===== OSC LAB2 =====\n");

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
            load_kernel_uart();
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}

int main(){
    shell();
    return 0;
}