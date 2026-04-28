#ifndef TIMER_H
#define TIMER_H

extern unsigned long CLOCK_FREQ;

// for add time event in line
struct timer_event {
    unsigned long expire_time;         
    void (*callback)(void*);
    void* arg; 
    struct timer_event *next;
};

unsigned long get_time();
void timer_init();
void fdt_timer_init(const void* fdt);
void add_timer(void (*callback)(void*), void* arg, int sec);
void do_setTimeout(char* sec_ptr, char* msg_ptr);
void timer_event_handler();

#endif