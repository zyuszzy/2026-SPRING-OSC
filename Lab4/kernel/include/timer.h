#ifndef TIMER_H
#define TIMER_H

extern unsigned long CLOCK_FREQ;

unsigned long get_time();
void timer_init();
void fdt_timer_init(const void* fdt);

#endif