# include "uart.h"
# include "trap.h"
# include "timer.h"
# include "plic.h"
# include "sbi.h"

void do_trap(struct pt_regs *regs){
    unsigned long scause = regs->scause;

    if(scause & (1ULL << 63)){
        unsigned long code = scause & 0xFFF;
        switch(code){
            case 5:
                static int sec = 0;
                sec += 2;
                uart_puts("[Timer] boot time: ");
                uart_putd(sec);
                uart_puts("\n");
                sbi_set_timer(get_time() + CLOCK_FREQ * 2);
                break;
            case 9:
                int irq = plic_claim();
                if(irq == UART_IRQ){
                    uart_isr();
                }
                if(irq)
                    plic_complete(irq); 
                break;
            default:
                uart_puts("Unknown Interrupt\n");
                break;
        }
    }else{
        uart_puts("=== S-mode trap ===");
        uart_puts("\nscause: "); uart_hex(regs->scause);
        uart_puts("\nsepc: "); uart_hex(regs->sepc);
        uart_puts("\nstval: "); uart_hex(regs->stval);
        uart_puts("\n");   

        if(scause == 8){     // ecall
            regs->sepc += 4;
        }
    }
}