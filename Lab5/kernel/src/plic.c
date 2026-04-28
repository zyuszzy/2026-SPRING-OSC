# include "plic.h"
# include "uart.h"
# include "fdt.h"
# include "string.h"

unsigned long PLIC_BASE;
extern int UART_IRQ;
extern unsigned long boot_cpu_hartid;

void fdt_plic_init(const void* fdt) {
    int node = fdt_node_offset_by_compatible(fdt, -1, "riscv,plic0");
    if (node < 0) {
        node = fdt_node_offset_by_compatible(fdt, -1, "sifive,plic-1.0.0");
    }

    if (node >= 0) {
        int len;
        const uint32_t *reg = fdt_getprop(fdt, node, "reg", &len);
        if (reg) {
            PLIC_BASE = ((uint64_t)bswap32(reg[0]) << 32) | bswap32(reg[1]);
            // uart_puts("[Kernel] DETECTED PLIC at: ");
            // uart_hex(PLIC_BASE);
            // uart_puts("\n");
        }
    } else {
        uart_puts("[Error] Could not find PLIC in FDT!\n");
    }
}

void plic_init() {
    *(volatile unsigned int*)PLIC_PRIORITY(UART_IRQ) = 1;    // (1) Set UART interrupt priority
    *(volatile unsigned int*)PLIC_ENABLE(boot_cpu_hartid, UART_IRQ) = (1 << UART_IRQ);      // tell PLIC, hart X can receive interrupt from UART;
    *(volatile unsigned int*)PLIC_THRESHOLD(boot_cpu_hartid) = 0;    // (3) Set threshold for the boot hart
}

int plic_claim() {
    int irq = *(volatile unsigned int*)PLIC_CLAIM(boot_cpu_hartid);
    return irq;
}

void plic_complete(int irq) {
    *(volatile unsigned int*)PLIC_CLAIM(boot_cpu_hartid) = irq;
}