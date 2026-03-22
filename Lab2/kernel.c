#include "uart.h"

void start_kernel() {
    uart_puts("Kernel wow yaya!!\n");
    while(1){};
}