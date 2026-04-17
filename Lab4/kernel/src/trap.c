# include "uart.h"
# include "trap.h"

void do_trap(struct pt_regs *regs){
    uart_puts("=== S-mode trap ===");
    uart_puts("\nscause: "); uart_hex(regs->scause);
    uart_puts("\nsepc: "); uart_hex(regs->sepc);
    uart_puts("\nstval: "); uart_hex(regs->stval);
    uart_puts("\n");

    if(regs->scause == 8){     // ecall
        regs->sepc += 4;
    }
}