#ifndef TASK_H
#define TASK_H

typedef void (*task_callback_t)(void *arg);
typedef struct {
    task_callback_t func;
    void *data;
    int priority;
} task_t;

void add_task(task_callback_t callback, void *arg, int priority);
void task_run();
void task_init();
void task_run_single();

#endif