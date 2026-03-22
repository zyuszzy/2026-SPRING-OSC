// #define UART_RBR  (volatile unsigned char*)(UART_BASE + (0x0 * UART_STRIDE))
// #define UART_THR  (volatile unsigned char*)(UART_BASE + (0x0 * UART_STRIDE))
// #define UART_LSR  (volatile unsigned char*)(UART_BASE + (0x5 * UART_STRIDE))
unsigned long UART_BASE = 0x10000000UL; 
int UART_STRIDE = 1;
#define UART_REG(offset) (volatile unsigned char *)(UART_BASE + (offset * UART_STRIDE))
#define LSR_DR    (1 << 0)
#define LSR_TDRQ  (1 << 5)

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
