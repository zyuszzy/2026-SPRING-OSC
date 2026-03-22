typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long      size_t;
typedef unsigned long      uintptr_t;
#define NULL ((void*)0)

#ifndef UART_H
#define UART_H

extern unsigned long UART_BASE;
extern int UART_STRIDE;

char uart_getc();
void uart_putc(char c);
void uart_puts(const char* s);
void uart_hex(unsigned long h);
void uart_putd(unsigned int n);
char uart_getc_raw();

#endif