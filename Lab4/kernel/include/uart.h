#ifndef UART_H
#define UART_H

#include "uart_hw.h"

#define RING_BUF_SIZE 512

extern int UART_IRQ;
extern unsigned long UART_BASE;
extern int UART_STRIDE;

typedef struct{
    unsigned char buffer[RING_BUF_SIZE];
    int head;   // for read
    int tail;   // for write
} ring_buffer_t;

char uart_getc();
void uart_putc(char c);
void uart_puts(const char* s);
void uart_hex(unsigned long h);
void uart_putd(unsigned int n);
char uart_getc_raw();
void fdt_uart_init(const void* fdt);

void uart_init();
void uart_isr();

#endif