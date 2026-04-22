#include "uart.h"
#include "uart_hw.h"
#include "string.h"
#include "task.h"
#include "shell.h"
#include "type.h"

/******************************************/
// #define UART_REG(offset) (volatile unsigned char *)(UART_BASE + (offset * UART_STRIDE))
// #define UART_RBR  (unsigned char*)(UART_BASE + 0x0)     // Receive Buffer Register
// #define UART_THR  (unsigned char*)(UART_BASE + 0x0)     // Transmit Holding Register
// #define UART_IER  (unsigned char*)(UART_BASE + 0x1)     // Interrupt Enable Register
// #define UART_IIR  (unsigned char*)(UART_BASE + 0x2)     // Interrupt Identification Register
// #define UART_MCR  (unsigned char*)(UART_BASE + 0x4)
// #define UART_LSR  (unsigned char*)(UART_BASE + 0x5)
/*****************************************/

int UART_IRQ;
unsigned long UART_BASE;
int UART_STRIDE;

void fdt_uart_init(const void* fdt){
    int uart_offset = fdt_path_offset(fdt, "/soc/serial");
    if(uart_offset >= 0){
        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, uart_offset, "reg", &len);

        // UART_BASE
        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);
    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | reg1;
            if (reg0 == 0) detected_base = reg1;    

            UART_BASE = detected_base; 

            // 判斷 STRIDE
            if (UART_BASE == 0x10000000UL) {
                UART_STRIDE = 1;        // QEMU
            } else {
                UART_STRIDE = 4;        // OrangePi
            }

            // uart_puts("[Kernel] DETECTED UART BASE: ");
            // uart_hex(detected_base);
            // uart_puts("\n");

        }else{
            uart_puts("[Kernel] Error: 'reg' property not found.\n");
        }

        // UART_IRQ
        uint32_t* irq_prop = (uint32_t*)fdt_getprop(fdt, uart_offset, "interrupts", &len);
        if (irq_prop) {
            UART_IRQ = bswap32(irq_prop[0]);
            
            // uart_puts("[Kernel] DETECTED UART IRQ: ");
            // uart_putd(UART0_IRQ);
            // uart_puts("\n");
        } else {
            uart_puts("[Kernel] Warning: 'interrupts' not found, using default 10\n");
        }
    }
}

ring_buffer_t rx_buf = { .head = 0, .tail = 0 };
static ring_buffer_t tx_buf = { .head = 0, .tail = 0 };

static inline int is_buf_empty(ring_buffer_t *b) {
    return (b->head == b->tail);
}
static inline int is_buf_full(ring_buffer_t *b) {
    return ((b->head + 1) % RING_BUF_SIZE == b->tail);
}
static inline unsigned long disable_irq(){
    unsigned long sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    asm volatile("csrci sstatus, 1 << 1");       // clear sstatus.SIE (bit1)
    return sstatus;
}
static inline void restore_irq(unsigned long sstatus){
    asm volatile("csrw sstatus, %0" : : "r"(sstatus));
}


void uart_init(){
    *UART_REG(0x1) |= 1;     // UART_IER RX(bit 0)
    *UART_REG(0x4) |= (1 << 3);       // Enable UART interrupt
}

int uart_is_readable() {
    return !is_buf_empty(&rx_buf);
}

void uart_isr(){
    // Read buffer
    while(*UART_REG(0x5) & LSR_DR){
        char c = (char)*UART_REG(0x0);
        
        if(!is_buf_full(&rx_buf)){
            rx_buf.buffer[rx_buf.head] = c;
            rx_buf.head = (rx_buf.head + 1) % RING_BUF_SIZE;
        }
    }
    task_add((task_func_t)shell_task_handler, NULL, 1);

    // write buffer
    if(*UART_REG(0x5) & LSR_TDRQ){      // if transmiter empty
        if(is_buf_empty(&tx_buf)){     // empty
            *UART_REG(0x1) &= ~(1 << 1);    // close TX interrupt temporary
        }else{
            *UART_REG(0x0) = tx_buf.buffer[tx_buf.tail];
            tx_buf.tail = (tx_buf.tail + 1) % RING_BUF_SIZE;
        }
    }
}


char uart_getc_raw(){
    char c;
    while(1){
        // ------------- Critical section -------------
        unsigned long s = disable_irq();
        if(rx_buf.head != rx_buf.tail){
            c = rx_buf.buffer[rx_buf.tail];
            rx_buf.tail = (rx_buf.tail + 1) % RING_BUF_SIZE;

            restore_irq(s);
        // ------------ Critical section End -----------
            return c;
        }

        // read buffer empty, open inerrupt and wait for data
        restore_irq(s);
        // ------------ Critical section End -----------
    }
}
char uart_getc(){
    char c = uart_getc_raw();
    return (c == '\r') ? '\n' : c;
}
int uart_getc_nonblocking(char *ptr) {
    unsigned long s = disable_irq();
    int status = 0;

    if (rx_buf.head != rx_buf.tail) {
        *ptr = rx_buf.buffer[rx_buf.tail];
        if(*ptr == '\r') *ptr = '\n';
        rx_buf.tail = (rx_buf.tail + 1) % RING_BUF_SIZE;
        status = 1; 
    }

    restore_irq(s);
    return status; 
}

void uart_putc(char c){
    if(c == '\n')
        uart_putc('\r');

    // ------------- Critical section -------------
    unsigned long s = disable_irq();

    while(is_buf_full(&tx_buf)){
        restore_irq(s);
    // ------------ Critical section End -----------
        s = disable_irq();
    }

    tx_buf.buffer[tx_buf.head] = c;
    tx_buf.head = (tx_buf.head + 1) % RING_BUF_SIZE;

    if ((*UART_REG(0x5) & LSR_TDRQ) && !is_buf_empty(&tx_buf)) {
        *UART_REG(0x0) = tx_buf.buffer[tx_buf.tail];
        tx_buf.tail = (tx_buf.tail + 1) % RING_BUF_SIZE;
    }

    *UART_REG(0x1) |= (1 << 1);     // open TX interrupt
    restore_irq(s);
    // ------------ Critical section End -----------   
}
void uart_puts(const char* s){
    while (*s) uart_putc(*s++);
}


void uart_hex(unsigned long h){
    char* table = "0123456789abcdef";
    for(int i=60 ; i>=0 ; i-=4){
        uart_putc(table[(h >> i) & 0xf]);
    }
}
void uart_putd(unsigned int n){
    if(n == 0){
        uart_putc('0');
        return;
    }
    char buf[16];
    int i = 0;
    while(n > 0){
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while(--i >= 0) 
        uart_putc(buf[i]);
}


// ---------------------------------------------------------------------------------------------
void uart_putc_pol(char c) {
    if (c == '\n') uart_putc_pol('\r');
    
    while (!(*UART_REG(0x5) & (1 << 5))); 
    *UART_REG(0x0) = c;
}
void uart_puts_pol(const char* s) {
    while (*s) uart_putc_pol(*s++);
}
void uart_hex_pol(unsigned long h) {
    uart_puts_pol("0x");
    for (int i = 60; i >= 0; i -= 4) {
        unsigned long digit = (h >> i) & 0xf;
        if (digit < 10) uart_putc_pol('0' + digit);
        else uart_putc_pol('a' + (digit - 10));
    }
}
void uart_putd_pol(unsigned int n) {
    if (n == 0) {
        uart_putc_pol('0');
        return;
    }
    char buf[16];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (--i >= 0) {
        uart_putc_pol(buf[i]);
    }
}