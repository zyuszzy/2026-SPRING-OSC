# include "uart_hw.h"
# include "type.h"

unsigned long UART_BASE;
int UART_STRIDE;

char uart_getc() {
    while ((*UART_REG(0x5) & LSR_DR) == 0);
    char c = (char)*UART_REG(0x0);
    return c == '\r' ? '\n' : c;
}

char uart_getc_raw() {
    while ((*UART_REG(0x5) & LSR_DR) == 0);
    return (char)*UART_REG(0x0);
}

void uart_putc(char c) {
    if (c == '\n')
        uart_putc('\r');

    while ((*UART_REG(0x5) & LSR_TDRQ) == 0);
    *UART_REG(0x0) = c;
}

void uart_puts(const char* s) {
    while (*s)
        uart_putc(*s++);
}

void uart_hex(unsigned long h) {
    uart_puts("0x");
    unsigned long n;
    for (int c = 60; c >= 0; c -= 4) {
        n = (h >> c) & 0xf;
        n += n > 9 ? 0x57 : '0';
        uart_putc(n);
    }
}

void uart_putd(unsigned int n){
    if (n == 0) {
        uart_putc('0');
        return;
    }
    char buf[10]; 
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}