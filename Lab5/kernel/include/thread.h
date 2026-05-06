#ifndef THREAD_H
#define THREAD_H

# include "trap.h"

#define STACK_SIZE 4096   // 4KB
#define TASK_RUNNING 0
#define TASK_ZOMBIE  1

#define MAX_SIG 32
#define SIGTERM 15

struct thread_struct {
    unsigned long ra;
    unsigned long sp;
    unsigned long s[12];    // Callee-saved registers(s0~s11)
};
struct task_struct {
    struct thread_struct context;
    int pid;
    int state;
    unsigned long kernel_sp;
    unsigned long user_sp;
    unsigned long stack_base;        // kerenl stack's start address
    unsigned long user_stack_base;
    void (*sig_handlers[MAX_SIG])();    // user registered function pointer
    unsigned int sig_pending; 
    struct pt_regs* saved_regs;        // When execute handler, save original Trap Frame
    unsigned long sig_stack_base;     // handler stack
    struct task_struct* prev;
    struct task_struct* next;
};


struct task_struct* get_current();
void schedule();
void thread_exit();
struct task_struct* thread_create(void (*threadfn)());
struct task_struct* user_process_create(void (*entry)());
struct task_struct* copy_process(struct pt_regs *parent_regs);
struct task_struct* find_task(int pid);
void wait_task(int pid);
void memcpy(void *dst, const void *src, unsigned long n);

#endif