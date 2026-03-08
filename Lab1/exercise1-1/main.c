#define UART_BASE 0x10000000UL
#define UART_RBR  (volatile unsigned char*)(UART_BASE + 0x0)     // Reciver Buffer Register(read-only)：接收到的字元
#define UART_THR  (volatile unsigned char*)(UART_BASE + 0x0)     // Transmitter Holding Register(write-only)：寫要發送的字元
#define UART_LSR  (volatile unsigned char*)(UART_BASE + 0x5)     // Line Status Register: 讀取 UART 的狀態(8-bit)

// LSR 狀態(bitmask)
#define LSR_DR    (1 << 0)      // Data Ready：1 表示RBR有資料可讀取
#define LSR_TDRQ  (1 << 5)      // Transmit Data Ready Queue：1 表示THR空的可寫入資料

// Read and return a single character received through the UART
char uart_getc() {
    while ((*UART_LSR & LSR_DR) == 0);
    return *UART_RBR;
}

// Transmit a single character through the UART.
void uart_putc(char c) {
    while ((*UART_LSR & LSR_TDRQ) == 0);
    *UART_THR = c;
    if (c == '\r') {
        while ((*UART_LSR & LSR_TDRQ) == 0);
        *UART_THR = '\n';
    }
}

// Transmit a string through the UART.
void uart_puts(const char* s) {
    while (*s != '\0'){
        uart_putc(*s);
        s++;
    }
}

void start_kernel() {
    uart_puts("\nStarting kernel ...\n");
    while (1) {
        uart_putc(uart_getc());
    }
}
