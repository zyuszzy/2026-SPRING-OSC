#ifndef THREAD_H
#define THREAD_H

#define STACK_SIZE 4096   // 4KB
#define TASK_RUNNING 0
#define TASK_ZOMBIE  1

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
    unsigned long stack_base;        // stack's start address
    struct task_struct* next;
};

struct task_struct* get_current();
void schedule();
void thread_exit();
struct task_struct* thread_create(void (*threadfn)());
struct task_struct* user_process_create(void (*entry)());
void wait_task(int pid);

#endif