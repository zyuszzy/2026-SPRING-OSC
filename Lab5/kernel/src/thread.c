# include "thread.h"
# include "mm.h"
# include "uart.h"
# include "trap.h"

static int next_pid = 0;
static int nr_threads = 0;
static struct task_struct* run_queue = 0;   // doubly
struct task_struct* zombie_queue = 0;    // singel LIFO list
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
void memcpy(void *dst, const void *src, unsigned long n) {
    char *d = dst;
    const char *s = src;
    while (n--) *d++ = *s++;
}
// ===================================================================================================================


// doubly circular link list
static void enqueue(struct task_struct** queue, struct task_struct* task) {
    // ----------------- Critical section -----------------
    unsigned long s = disable_irq();
    if (*queue == 0) {
        *queue = task;
        task->next = task;
        task->prev = task;
    } else {
        struct task_struct* tail = (*queue)->prev;
        tail->next = task;
        task->prev = tail;
        task->next = *queue;
        (*queue)->prev = task;
    }
    restore_irq(s);
    // ---------------- Critical section End --------------
}
void dequeue(struct task_struct* task) {
    if (!task) return;
    unsigned long s = disable_irq();
    
    task->prev->next = task->next;
    task->next->prev = task->prev;
    
    // if remove head
    if (run_queue == task) {
        if (task->next == task) run_queue = 0;
        else run_queue = task->next;
    }
    
    task->next = 0;
    task->prev = 0;
    restore_irq(s);
}

// take task_struct
struct task_struct* get_current() {
    register struct task_struct* current asm("tp");     // take current task_struct addr from tp(thread pointer)
    return current;
}

void schedule() {
    // ----------------- Critical section -----------------
    unsigned long s = disable_irq(); 

    if (get_current() == 0) uart_puts("Error: tp is zero!\n"); 
    if(!run_queue){
        restore_irq(s);
        return;
    }
    struct task_struct* curr = get_current();
    if(curr == run_queue) run_queue = run_queue ->next;
    if (curr != run_queue) {
        // 更新 tp 暫存器，讓 get_current() 在切換後能拿到正確的 next
        asm volatile("mv tp, %0" : : "r"(run_queue));
        // 行上下文切換
        switch_to(curr, run_queue);
    }

    restore_irq(s);
}
void kill_zombies(){
    if (zombie_queue == 0) return;

    // ----------------- Critical section -----------------
    unsigned long s = disable_irq();
    struct task_struct* curr = zombie_queue;
    zombie_queue = 0;
    restore_irq(s);
    // ---------------- Critical section End --------------

    while (curr) {
        struct task_struct* next_zombie = curr->next;
      
        // free memory
        if (curr->stack_base) free((void*)curr->stack_base);            // kernel stack
        if (curr->user_stack_base) free((void*)curr->user_stack_base);  // user stack
        if (curr->saved_regs) free(curr->saved_regs);                 // signal backup
        if (curr->sig_stack_base) free((void*)curr->sig_stack_base);   // signal stack
        
        free(curr);     // free task structure
        nr_threads--;
        curr = next_zombie;
    }
}

struct task_struct* thread_create(void (*threadfn)()){
    struct task_struct* task = (struct task_struct*)allocate(sizeof(struct task_struct));
    task->pid = next_pid++; 
    nr_threads++;
    task->state = TASK_RUNNING;
    
    task->stack_base = (unsigned long)allocate(STACK_SIZE);
    task->context.ra = (unsigned long)threadfn;
    task->context.sp = task->stack_base + STACK_SIZE;

    task->user_stack_base = 0;
    task->sig_pending = 0;
    task->sig_stack_base = 0;
    task->saved_regs = NULL;
    for (int i = 0; i < MAX_SIG; i++) task->sig_handlers[i] = NULL;
    
    task->prev = 0;
    task->next = 0;
    
    enqueue(&run_queue, task);
    return task;
}
struct task_struct* user_process_create(void (*entry)()){
    struct task_struct* task = (struct task_struct*)allocate(sizeof(struct task_struct));
    task->pid = next_pid++; 
    nr_threads++;
    task->state = TASK_RUNNING;

    // kernel stack & user stack
    task->stack_base = (unsigned long)allocate(STACK_SIZE);     // kernel stack
    task->kernel_sp = task->stack_base + STACK_SIZE;
    task->user_stack_base = (unsigned long)allocate(STACK_SIZE);
    task->user_sp = task->user_stack_base + STACK_SIZE;

    // signal init
    task->sig_pending = 0;  
    task->sig_stack_base = 0;
    task->saved_regs = NULL;
    for(int i = 0; i < MAX_SIG; i++) {
        task->sig_handlers[i] = NULL;
    }

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
struct task_struct* copy_process(struct pt_regs *parent_regs){

    // allocate child's task structure
    struct task_struct* child = (struct task_struct*)allocate(sizeof(struct task_struct));
    child->pid = next_pid++;
    nr_threads++;
    child->state = TASK_RUNNING;

    // allocate child's stack place
    child->stack_base = (unsigned long)allocate(STACK_SIZE);    //kernel stack base
    child->kernel_sp = child->stack_base + STACK_SIZE;
    child->user_stack_base = (unsigned long)allocate(STACK_SIZE);
    child->user_sp = child->user_stack_base + STACK_SIZE;


    // copy parent's stack
    struct task_struct* parent = get_current();
    memcpy((void*)(child->user_sp - STACK_SIZE), (void*)(parent->user_sp - STACK_SIZE), STACK_SIZE);    //user stack
    memcpy((void*)(child->kernel_sp - STACK_SIZE), (void*)(parent->kernel_sp - STACK_SIZE), STACK_SIZE);    // kernel stack

    unsigned long kstack_offset = parent->kernel_sp - (unsigned long)parent_regs;
    struct pt_regs* child_regs = (struct pt_regs*)(child->kernel_sp - kstack_offset);

    child->sig_pending = 0;    
    child->sig_stack_base = 0; 
    child->saved_regs = NULL;  
    for (int i = 0; i < MAX_SIG; i++) {
        child->sig_handlers[i] = parent->sig_handlers[i]; 
    }


    // adjuct child process's para
    child_regs->a0 = 0;     // returen value;

    unsigned long usp_offset = parent->user_sp - parent_regs->sp;
    child_regs->sp = child->user_sp - usp_offset;

    child_regs->tp = (unsigned long)child;  // task structure
    
    // adjust child's context
    extern void ret_from_exception();
    child->context.ra = (unsigned long)ret_from_exception;
    child->context.sp = (unsigned long)child_regs;

    enqueue(&run_queue, child);
    return child;
}
void thread_exit() {
    unsigned long s = disable_irq();
    struct task_struct* current = get_current();
    current->state = TASK_ZOMBIE;
    dequeue(current);
    current->next = zombie_queue;
    zombie_queue = current;  
    restore_irq(s);
    schedule();
}


struct task_struct* find_task(int pid) {
    unsigned long s = disable_irq();
    struct task_struct* curr = run_queue;
    if (!curr){
        restore_irq(s);
        return 0;
    }
    do {
        if (curr->pid == pid){
            restore_irq(s);
            return curr;
        }
        curr = curr->next;
    } while (curr != run_queue);
    restore_irq(s);
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
        /*uart_puts("Run_queue num: ");
        uart_putd((unsigned long)nr_threads);
        uart_puts("\n");*/
        kill_zombies();
        schedule();
    }
}
void foo() {
    for (int i = 0; i < 5; i++) {
        struct task_struct* temp = get_current();
        do{
            uart_putd(temp->pid);
            uart_puts("->");
            temp = temp->next;
        }while(temp != get_current());
        uart_puts("\n");

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