# include "shell.h"

void exec(const char* filename){
    unsigned int size;
    void* program_addr = find_user_program((void*)initrd_start, filename, &size);
    if(program_addr == NULL){
        uart_puts("exec: file not found.\n");
        return;
    }

    uart_puts("Loading program at: ");
    uart_hex((unsigned long)program_addr);
    uart_puts("\n");
}

void kernel_shell(){
    char input_buffer[64];
    int input_index = 0;
    
    // read user input
    while(1){
        uart_puts("opi-rv2> ");
        input_index = 0;
    }
}