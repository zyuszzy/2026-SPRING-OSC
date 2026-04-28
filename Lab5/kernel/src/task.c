# include "task.h"
# include "type.h"
# include "uart_hw.h"
# include "timer.h"

#define MAX_TASKS 64

extern void shell_task_handler(void* data);

static task_t task_queue[MAX_TASKS];
static int task_count = 0;
int current_task_priority = 9999;

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
        unsigned long s = disable_irq();
        if(task_count == 0 || task_queue[0].priority >= current_task_priority){
            restore_irq(s);
            break;
        }
    

        int prev_priority = current_task_priority;
        task_t t = task_queue[0];
        current_task_priority = t.priority;

        for(int i=0 ; i < task_count - 1 ; i++){
            task_queue[i] = task_queue[i + 1];
        }
        task_count--;

        asm volatile("csrsi sstatus, 1 << 1");  
        if(t.func != NULL){
            t.func(t.data);
        }
        asm volatile("csrci sstatus, 1 << 1"); 

        /*if(t.func == shell_task_handler){
            *UART_REG(0x1) |= 1;
        }else if(t.func == timer_event_handler){
            asm volatile(
            "li t0, (1 << 9);"
            "csrs sie, t0;");
        }*/
        current_task_priority = prev_priority;
        restore_irq(s);
    }
}
