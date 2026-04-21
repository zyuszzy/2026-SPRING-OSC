#ifndef UART_H
#define UART_H

#include "uart_hw.h"

#define RING_BUF_SIZE 2048

extern int UART_IRQ;
extern unsigned long UART_BASE;
extern int UART_STRIDE;

typedef struct{
    unsigned char buffer[RING_BUF_SIZE];
    volatile int head;   // for read
    volatile int tail;   // for write
} ring_buffer_t;

char uart_getc();
void uart_putc(char c);
void uart_puts(const char* s);
void uart_hex(unsigned long h);
void uart_putd(unsigned int n);
char uart_getc_raw();
void fdt_uart_init(const void* fdt);
void uart_putc_pol(char c);
void uart_puts_pol(const char* s);

void uart_init();
void uart_isr();

#endif