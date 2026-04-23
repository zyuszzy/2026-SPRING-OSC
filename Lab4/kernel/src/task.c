# include "task.h"

#define MAX_TASKS 64

static task_t task_queue[MAX_TASKS];
static int task_count = 0;

static inline unsigned long disable_irq(){
    unsigned long sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    asm volatile("csrci sstatus, 1 << 1");       // clear sstatus.SIE (bit1)
    return sstatus;
}
static inline void restore_irq(unsigned long sstatus){
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
}

void task_init(){
    task_count = 0;
}

void add_task(task_callback_t callback, void *arg, int priority){

    // ----------------------- Critical section ---------------------
    unsigned long s = disable_irq(); 

    if(task_count >= MAX_TASKS){
        restore_irq(s); 
    // --------------------- Critical section end -------------------
        return;
    }

    // add into task queue
    int i = task_count - 1;
    while(i >= 0 && task_queue[i].priority > priority){
        task_queue[i + 1] = task_queue[i];
        i--;
    }

    task_queue[i + 1].func = callback;
    task_queue[i + 1].data = arg;
    task_queue[i + 1].priority = priority;   
    task_count++; 

    restore_irq(s);
    // --------------------- Critical section end -------------------
}

void task_run(){
    while(1){
        // ----------------------- Critical section ---------------------
        unsigned long s = disable_irq();
        if(task_count == 0){
            restore_irq(s);
        // --------------------- Critical section end -------------------
            continue;
        }

        task_t t = task_queue[0];
        for(int i=0 ; i < task_count - 1 ; i++){
            task_queue[i] = task_queue[i + 1];
        }
        task_count--;

        restore_irq(s); 
        // -------------------- Critical section end ------------------
        t.func(t.data); 
    }
}
void task_run_single() {
    unsigned long s = disable_irq();
    if(task_count == 0){
        restore_irq(s);
        return;
    }

    task_t t = task_queue[0];
    for(int i=0 ; i < task_count - 1 ; i++){
        task_queue[i] = task_queue[i + 1];
    }
    task_count--;

    restore_irq(s); 
    t.func(t.data);
}