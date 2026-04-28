#ifndef UART_HW__H
#define UART_HW__H

#include "type.h"

#define UART_REG(offset) (volatile unsigned char *)(UART_BASE + (offset * UART_STRIDE))
#define LSR_DR    (1 << 0)
#define LSR_TDRQ  (1 << 5)

extern unsigned long UART_BASE;
extern int UART_STRIDE;

#endif