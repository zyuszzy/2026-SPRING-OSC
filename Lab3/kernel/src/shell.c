# include "shell.h"

void kernel_shell(){
    char input_buffer[64];
    int input_index = 0;
    
    // 讀取使用者輸入
    while(1){
        uart_puts("# ");
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

    }
}