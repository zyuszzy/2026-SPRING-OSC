# include "thread.h"
# include "mm.h"
# include "uart.h"
# include "trap.h"

static int nr_threads = 0;
static struct task_struct* run_queue = 0;
extern void switch_to(struct task_struct* prev, struct task_struct* next);

static inline unsigned long disable_irq(){
    unsigned long sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    asm volatile("csrci sstatus, 1 << 1");       // clear sstatus.SIE (bit1)
    return sstatus;
}
static inline void restore_irq(unsigned long sstatus){
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
}

// circular link list
static void enqueue(struct task_struct** queue, struct task_struct* task) {
    // ----------------- Critical section -----------------
    unsigned long s = disable_irq();
    if (*queue == 0) {
        *queue = task;
        task->next = task;
    } else {
        struct task_struct* tail = (*queue)->next;      // insert new task behind current (curr -> n) => (curr -> new -> n)
        (*queue)->next = task;                                 
        task->next = tail;
    }
    restore_irq(s);
    // ---------------- Critical section End --------------
}

// take task_struct
struct task_struct* get_current() {
    register struct task_struct* current asm("tp");     // take current task_struct addr from tp(thread pointer)
    return current;
}

void schedule() {
    struct task_struct* current = get_current();
    struct task_struct* next = current->next;

    // find RUNNING task
    while(next->state != TASK_RUNNING && next != current){
        next = next->next;
    }
    
    if(next != current){
        asm volatile("move tp, %0" : : "r"(next));
        switch_to(current, next);
    }
}
void kill_zombies(){
    // ----------------- Critical section -----------------
    unsigned long s = disable_irq();
    if(run_queue == 0){
        restore_irq(s);
        return;
    }

    struct task_struct *prev = run_queue;
    struct task_struct *curr = run_queue->next;
    int count = nr_threads; 

    while(count--){
        if(curr->state == TASK_ZOMBIE){
            struct task_struct *zombie = curr;
            
            prev->next = curr->next;
            
            // If delete current run_queue head
            if(zombie == run_queue){
                run_queue = prev;
            }

            // last node?
            if(prev == zombie){
                run_queue = 0;
            }

            curr = curr->next;

            if(zombie->stack_base){
                free((void*)zombie->stack_base);
            }
            free(zombie);
             nr_threads--;

            if(run_queue == 0)
                break;
        }else{
            prev = curr;
            curr = curr->next;
        }
        if (curr == run_queue->next) break;
    }
    restore_irq(s); 
    // ---------------- Critical section End --------------
}

struct task_struct* thread_create(void (*threadfn)()){
    struct task_struct* task = (struct task_struct*)allocate(sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->state = TASK_RUNNING;
    
    task->stack_base = (unsigned long)allocate(STACK_SIZE);
    task->context.ra = (unsigned long)threadfn;
    task->context.sp = task->stack_base + STACK_SIZE; 
    
    enqueue(&run_queue, task);
    return task;
}
struct task_struct* user_process_create(void (*entry)()){
    struct task_struct* task = (struct task_struct*)allocate(sizeof(struct task_struct));
    task->pid = nr_threads++;
    task->state = TASK_RUNNING;

    // kernel stack & user stack
    task->stack_base = (unsigned long)allocate(STACK_SIZE);     // kernel stack
    task->kernel_sp = task->stack_base + STACK_SIZE;
    task->user_sp = (unsigned long)allocate(STACK_SIZE) + STACK_SIZE;

    // space for trap frame from kernel stack
    struct pt_regs* regs = (struct pt_regs*)(task->kernel_sp - sizeof(struct pt_regs));
    
    // Init
    for (int i = 0; i < sizeof(struct pt_regs) / 8; i++) ((unsigned long*)regs)[i] = 0;

    // Trap Frame
    regs->tp = (unsigned long)task;
    regs->sepc = (unsigned long)entry;      // user space entry
    regs->sp = task->user_sp;               // user stack
    
    unsigned long sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    sstatus &= ~(1 << 8);       // SPP = 0 (User Mode)
    sstatus |= (1 << 5);        // SPIE = 1 (Interrupt when user mode)
    regs->sstatus = sstatus;

    // Context (for switch_to)
    extern void ret_from_exception(); 
    task->context.ra = (unsigned long)ret_from_exception;
    task->context.sp = (unsigned long)regs;               

    enqueue(&run_queue, task);
    return task;
}
void thread_exit() {
    struct task_struct* current = get_current();
    current->state = TASK_ZOMBIE;
    schedule();
}


struct task_struct* find_task(int pid) {
    struct task_struct* curr = run_queue;
    if (!curr) return 0;
    do {
        if (curr->pid == pid) return curr;
        curr = curr->next;
    } while (curr != run_queue);
    return 0;
}
void wait_task(int pid) {
    while (1) {
        struct task_struct* target = find_task(pid);
        if (target == 0 || target->state == TASK_ZOMBIE) {
            break;
        }
        schedule();
    }
}

void idle() {
    while (1) {
        kill_zombies();
        schedule();
    }
}
void foo() {
    for (int i = 0; i < 5; i++) {
        uart_puts("Thread id: ");
        uart_putd(get_current()->pid);
        uart_puts(" ");
        uart_putd(i);
        uart_puts("\n");
        for (int j = 0; j < 100000000; j++);
        schedule();
    }
    thread_exit();
}