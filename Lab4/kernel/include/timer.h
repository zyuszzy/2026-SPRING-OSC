#ifndef TIMER_H
#define TIMER_H

extern unsigned long CLOCK_FREQ;

void timer_init();
unsigned long get_time();
void fdt_timer_init(const void* fdt);

#endif