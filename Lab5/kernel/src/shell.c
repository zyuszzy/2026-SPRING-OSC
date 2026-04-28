# include "type.h"
# include "string.h"
# include "uart.h"
# include "initrd.h"
# include "timer.h"
# include "mm.h"
# include "fdt.h"

extern boot_info_t info;
static char input_buffer[64];
static int input_index = 0;
extern int current_task_priority;



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
        current_task_priority = 9999;
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
        uart_puts("exec: ");
        uart_puts(input);
        uart_puts("file not found.\n");
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

void execute_command(char* input_buffer){
    if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  exec <User program name>  - run user program.\n");
            uart_puts("  setTimeout <SEC> <MSG> - print MSG after SEC seconds.\n");
            uart_puts("  test - demo test.\n");
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

void shell_task_handler(void* data){

    char input_c;
    while(uart_getc_nonblocking(&input_c)){
        if(input_c == '\b' || input_c == 127){
            if(input_index > 0){
                input_index--;
                uart_puts("\b \b");
            }
            continue;
        }

        uart_putc(input_c);

        if(input_c == '\n' || input_c == '\r'){
            if (input_index > 0) { 
                input_buffer[input_index] = '\0'; 
                input_index = 0;
                execute_command(input_buffer);
                uart_puts("opi-rv2> ");
            }
            continue;
        }
        if(input_index < 63 && input_c >= 32 && input_c <= 126){
            input_buffer[input_index++] = input_c;
        }
    }
}

void shell_init(){
    uart_puts("Starting Kernel...\n");
    uart_puts("===== OSC LAB4 =====\n");
    uart_puts("opi-rv2> ");
    input_index = 0;
}
