#ifndef PLIC_H
#define PLIC_H

extern unsigned long PLIC_BASE;

#define PLIC_PRIORITY(irq)   (PLIC_BASE + (irq) * 4)
#define PLIC_ENABLE(hart, irq)    (PLIC_BASE + 0x002080 + (hart) * 0x0100 + ((irq) / 32) * 4)   // enable specific hart to receive
#define PLIC_THRESHOLD(hart) (PLIC_BASE + 0x201000 + (hart) * 0x2000)
#define PLIC_CLAIM(hart)     (PLIC_BASE + 0x201004 + (hart) * 0x2000)

void fdt_plic_init(const void* fdt);
void plic_init();
int plic_claim();
void plic_complete(int irq);

#endif