# include "uart.h"
# include "trap.h"
# include "timer.h"
# include "plic.h"
# include "sbi.h"
# include "mm.h"

extern void syscall_handler(struct pt_regs *regs);

void do_trap(struct pt_regs *regs){
    unsigned long scause = regs->scause;

    if(scause & (1ULL << 63)){
        unsigned long code = scause & 0xFFF;
        switch(code){
            case 5:     // timer interrupt
                /*asm volatile(
                    "li t0, (1 << 5);"
                    "csrc sie, t0;");*/
                timer_event_handler();
                break;
            case 9:     // UART
                int irq = plic_claim();
                if(irq == UART_IRQ){
                    uart_isr();
                }
                if(irq > 0){
                    plic_complete(irq); 
                }
                break;
            default:
                uart_puts("Unknown Interrupt\n");
                break;
        }
        //task_run(); 
    }else{
        if(scause == 8){     // ecall
            regs->sepc += 4;
            syscall_handler(regs);
        }else{
            uart_puts("=== S-mode trap ===");
            uart_puts("\nscause: "); uart_hex(regs->scause);
            uart_puts("\nsepc: "); uart_hex(regs->sepc);
            uart_puts("\nstval: "); uart_hex(regs->stval);
            uart_puts("\n");   
        }
    }

}