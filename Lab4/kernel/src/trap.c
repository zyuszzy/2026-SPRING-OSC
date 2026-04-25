# include "uart.h"
# include "trap.h"
# include "timer.h"
# include "plic.h"
# include "sbi.h"
# include "mm.h"

int nested_level = 0;

void do_trap(struct pt_regs *regs){
    nested_level++;
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
        //asm volatile("csrsi sstatus, 1 << 1");  // enable interrupt
        /*if (nested_level > 0) {
            uart_puts_pol("[Debug] Nested Interrupt! Level: ");
            uart_putd_pol(nested_level);
            uart_puts_pol("\n"); 
        }*/
        
        task_run(); 
        //asm volatile("csrci sstatus, 1 << 1");  // disable interrupt
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

    //task_run_single(); 

}