# include "uart.h"
# include "trap.h"
# include "timer.h"
# include "plic.h"
# include "sbi.h"
# include "mm.h"
# include "thread.h"

extern void syscall_handler(struct pt_regs *regs);

void handle_signal(struct pt_regs *regs) {
    struct task_struct *curr = get_current();
    
    // return to kernel mode
    if ((regs->sstatus & (1 << 8)) != 0) return;

    if (curr->saved_regs != 0) return;

    if (curr->sig_pending == 0) return;


    for (int signum = 0; signum < MAX_SIG; signum++) {
        if (curr->sig_pending & (1 << signum)) {
            
            void (*handler)() = curr->sig_handlers[signum];
            
            // clean signal pending
            curr->sig_pending &= ~(1 << signum);

            if (handler) {
                // haved register handler, jump to handler

                // backup trape frame
                curr->saved_regs = (struct pt_regs *)allocate(sizeof(struct pt_regs));
                memcpy(curr->saved_regs, regs, sizeof(struct pt_regs));

                // signal stack
                curr->sig_stack_base = (unsigned long)allocate(STACK_SIZE);
                
                // Trampoline (stack top)
                uint32_t *tramp = (uint32_t *)curr->sig_stack_base;
                tramp[0] = 0x00b00893;      // li a7, 11(sigreturn)
                tramp[1] = 0x00000073;      // ecall

                // go to handler
                regs->sepc = (unsigned long)handler;
                regs->ra = curr->sig_stack_base;
                regs->sp = curr->sig_stack_base + STACK_SIZE; 

                return; 
            } else {
                uart_puts("Signal received but no handler, exiting...\n");
                thread_exit(); 
                return;
            }
        }
    }
}

void do_trap(struct pt_regs *regs){
    unsigned long scause = regs->scause;

    if(scause & (1ULL << 63)){
        unsigned long code = scause & 0xFFF;
        switch(code){
            case 5:     // timer interrupt
                /*asm volatile(
                    "li t0, (1 << 5);"
                    "csrc sie, t0;");*/
                sbi_set_timer(get_time()+(CLOCK_FREQ / 32));
                schedule();
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
            asm volatile("csrsi sstatus, 1 << 1");
            syscall_handler(regs);
            asm volatile("csrci sstatus, 1 << 1");
        }else{
            uart_puts("=== S-mode trap ===");
            uart_puts("\nscause: "); uart_hex(regs->scause);
            uart_puts("\nsepc: "); uart_hex(regs->sepc);
            uart_puts("\nstval: "); uart_hex(regs->stval);
            uart_puts("\n");   
        }
    }

    handle_signal(regs);
}