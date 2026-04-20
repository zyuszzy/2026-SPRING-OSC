# include "type.h"
# include "string.h"
# include "uart.h"
# include "initrd.h"
# include "timer.h"
# include "mm.h"
# include "fdt.h"

extern boot_info_t info;

void exec(char* input){
    
    // only type exec or didn't type program namm
    if(strcmp(input, "exec") == 0 || strcmp(input, "exec ") == 0){
        uart_puts("Usage: exec <filename>\n");
        return;
    }

    char* filename = input + 5;
    unsigned int size;
    void* user_entry = find_user_program((void*)info.initrd_start, filename, &size);
    if(user_entry){
        void* stack_page = allocate(PAGE_SIZE);        
        void* user_sp = (void*)((unsigned long)stack_page + PAGE_SIZE);

        // save kernel sp
        unsigned long kernel_sp;
        asm volatile("mv %0, sp" : "=r"(kernel_sp)); 
        asm volatile("csrw sscratch, %0" : : "r"(kernel_sp));

        // set sepc & save user sp
        asm volatile("csrw sepc, %0" : : "r"(user_entry));
        asm volatile("mv sp, %0" : : "r"(user_sp));

        // set sstatus
        unsigned long sstatus;
        asm volatile("csrr %0, sstatus" : "=r"(sstatus));
        sstatus &= ~(1 << 8);   // SPP
        sstatus |= (1 << 5);    // SPIE
        asm volatile("csrw sstatus, %0" : : "r"(sstatus));
        asm volatile("sret");
    }else{
        uart_puts("exec: file not found.\n");
        return;
    }
}

void settimeout(char *input){
    // only type command
    if(strcmp(input, "setTimeout") == 0 || strcmp(input, "setTimeout ") == 0){
        uart_puts("Usage: setTimeout <SEC> <MSG>\n");
        return;
    }

    char *sec_ptr = input + 11;
    char *msg_ptr = sec_ptr;
    while(*msg_ptr != ' ' && *msg_ptr != '\0'){
        msg_ptr++;
    }

    if(*msg_ptr == ' '){
        *msg_ptr = '\0';    // delete ' ', then sec_ptr become a string
        msg_ptr++;
        do_setTimeout(sec_ptr, msg_ptr);
    }else{
        uart_puts("Usage: setTimeout <SECONDS> <MESSAGE>\n");
    }
}

void kernel_shell(){
    uart_puts("Starting Kernel...\n");
    uart_puts("===== OSC LAB3 =====\n");
    char input_buffer[64];
    int input_index = 0;
    
    // read user input
    while(1){
        uart_puts("opi-rv2> ");
        input_index = 0;

        // get all line input
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

        if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  exec <User program name>  - run user program.\n");
            uart_puts("  setTimeout <SEC> <MSG> - print MSG after SEC seconds.\n");
        }else if(strncmp(input_buffer, "exec", 4) == 0){        // If exec, goto advanced judge
            exec(input_buffer);
        }else if(strncmp(input_buffer, "setTimeout", 10) == 0){
            settimeout(input_buffer);
        }
        else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}