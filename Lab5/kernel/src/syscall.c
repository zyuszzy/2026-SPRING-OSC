#include "trap.h"
#include "thread.h"
#include "initrd.h"
#include "fdt.h"
#include "mm.h"
#include "uart.h"

#define SYS_GETPID     0
#define SYS_UART_READ  1
#define SYS_UART_WRITE 2
#define SYS_EXEC       3
#define SYS_FORK       4
#define SYS_WAITPID    5
#define SYS_EXIT       6
#define SYS_STOP       7

extern boot_info_t info;

static inline unsigned long disable_irq(){
    unsigned long sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    asm volatile("csrci sstatus, 1 << 1");       // clear sstatus.SIE (bit1)
    return sstatus;
}
static inline void restore_irq(unsigned long sstatus){
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
}

// 0: long getpid()
void sys_getpid(struct pt_regs *regs){
    regs->a0 = get_current()->pid; 
}
// 1: long uart_read(char *buf, long count)
void sys_uart_read(struct pt_regs *regs){
    char* buf = (char*)regs->a0;    // user gives buffer addr.
    long count = regs->a1;       // read num
    long i;
    for (i = 0; i < count; i++){
        buf[i] = uart_getc(); 
    }
    regs->a0 = i;
}
// 2: long uart_write(const char *buf, long count)
void sys_uart_write(struct pt_regs *regs){
    char* buf = (char*)regs->a0;    // user gives buffer addr
    long count = regs->a1;      // write num
    long written = 0;
    for(long i = 0; i < count; i++){
        uart_putc(buf[i]);
        written++;
    }
    regs->a0 = written;
}
// 3: int exec(const char *path)
void sys_exec(struct pt_regs *regs){
    const char* path = (const char*)regs->a0;
    unsigned int size;
    void* user_entry = find_user_program((void*)info.initrd_start, path, &size);
    
    if(user_entry){
        regs->sepc = (unsigned long)user_entry;
        regs->sp = (unsigned long)allocate(STACK_SIZE) + STACK_SIZE; 
        regs->a0 = 0;
    }else{
        regs->a0 = -1;
    }
}
// 4: long fork()
void sys_fork(struct pt_regs *regs){
    // ---------------- Critical section --------------------
    unsigned long s = disable_irq();

    struct task_struct* child = copy_process(regs);
    regs->a0 = child->pid;

    restore_irq(s);
    // --------------- Critical section End ------------------
}
// 5: long waitpid(long pid)
void sys_waitpid(struct pt_regs *regs){
    long target_pid = regs->a0;
    wait_task(target_pid);
    regs->a0 = target_pid;      // return end pid
}
// 6: void exit(int status)
void sys_exit(struct pt_regs *regs) {
    thread_exit(); 
}
// 7: int stop(long pid) 
void sys_stop(struct pt_regs *regs) {
    long target_pid = regs->a0;
    struct task_struct* target = find_task(target_pid);
    
    if(target){
        target->state = TASK_ZOMBIE;
        regs->a0 = 0;
    }else{
        regs->a0 = -1; 
    }
}

void syscall_handler(struct pt_regs *regs){
    unsigned long syscall_num = regs->a7;

    // unsigned long tp_val;
    // asm volatile("mv %0, tp" : "=r"(tp_val));
    // uart_puts("Current TP: "); uart_hex(tp_val); uart_puts("\n");

    // uart_puts("Syscall ID received: ");
    // uart_putd(syscall_num);
    // uart_puts("\n");

    switch(syscall_num){
        case SYS_GETPID:  sys_getpid(regs); break;
        case SYS_UART_READ: sys_uart_read(regs); break;
        case SYS_UART_WRITE: sys_uart_write(regs); break;
        case SYS_EXEC: sys_exec(regs); break;
        case SYS_FORK: sys_fork(regs); break;
        case SYS_WAITPID: sys_waitpid(regs); break;
        case SYS_EXIT: sys_exit(regs); break;
        case SYS_STOP: sys_stop(regs); break;
        default:
            uart_puts("Unknown syscall\n");
            break;
    }
}