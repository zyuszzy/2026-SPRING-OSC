#ifndef PLIC_H
#define PLIC_H

extern unsigned long PLIC_BASE;

#define PLIC_PRIORITY(irq)   (PLIC_BASE + (irq) * 4)
#define PLIC_ENABLE(hart)    (PLIC_BASE + 0x002080 + (hart) * 0x0100)   // enable specific hart to receive
#define PLIC_THRESHOLD(hart) (PLIC_BASE + 0x201000 + (hart) * 0x2000)
#define PLIC_CLAIM(hart)     (PLIC_BASE + 0x201004 + (hart) * 0x2000)

void plic_init();
int plic_claim();
void plic_complete(int irq);

#endif