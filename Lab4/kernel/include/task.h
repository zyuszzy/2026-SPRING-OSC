#ifndef TASK_H
#define TASK_H

typedef void (*task_func_t)(void*);
typedef struct {
    task_func_t func;    
    void* data;          
    int priority;       
} task_t;

void task_add(task_func_t func, void* data, int priority);
void task_run();
void task_init();
void task_run_single();

#endif