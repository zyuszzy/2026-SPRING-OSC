#include "trap.h"
#include "thread.h"
#include "uart.h"

#define SYS_GETPID 0
#define SYS_UART_WRITE 2

void sys_getpid(struct pt_regs *regs){
    regs->a0 = get_current()->pid; 
}
void sys_uart_write(struct pt_regs *regs){
    char* buf = (char*)regs->a0;
    long count = regs->a1;
    long written = 0;

    for(long i = 0; i < count; i++){
        uart_putc(buf[i]);
        written++;
    }
    regs->a0 = written;
}

void syscall_handler(struct pt_regs *regs){
    unsigned long syscall_num = regs->a7;

    // unsigned long tp_val;
    // asm volatile("mv %0, tp" : "=r"(tp_val));
    // uart_puts("Current TP: "); uart_hex(tp_val); uart_puts("\n");

    switch(syscall_num){
        case SYS_GETPID:  sys_getpid(regs); break;
        case SYS_UART_WRITE: sys_uart_write(regs); break;
        default:
            uart_puts("Unknown syscall\n");
            break;
    }
}