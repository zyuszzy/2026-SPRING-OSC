#ifndef UART_H
#define UART_H

#include "type.h"

extern unsigned long UART_BASE;
extern int UART_STRIDE;

char uart_getc();
void uart_putc(char c);
void uart_puts(const char* s);
void uart_hex(unsigned long h);
void uart_putd(unsigned int n);
char uart_getc_raw();

#endif