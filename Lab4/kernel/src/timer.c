# include "timer.h"
# include "type.h"
# include "uart.h"
# include "string.h"
# include "fdt.h"
# include "sbi.h"

unsigned long CLOCK_FREQ;

unsigned long get_time(){
    unsigned long n;
    asm volatile("rdtime %0" : "=r"(n));
    return n;
}

void timer_init(){
    unsigned long target = get_time() + (CLOCK_FREQ)*2;     // 2secs
    sbi_set_timer(target);

    // sie.STIE
    // open accept timer inerrupt
    unsigned long sie;
    asm volatile("csrr %0, sie" : "=r"(sie));
    sie |= (1 << 5); 
    asm volatile("csrw sie, %0" : : "r"(sie));

    // sstatus.SIE
    // unsigned long sstatus;
    // asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    // sstatus |= (1 << 1); 
    // asm volatile("csrw sstatus, %0" : : "r"(sstatus));
}

void fdt_timer_init(const void* fdt){
    int cpus_offset = fdt_path_offset(fdt, "/cpus");
    if(cpus_offset >= 0){
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(fdt, cpus_offset, "timebase-frequency", &len);
        if(prop){
            CLOCK_FREQ = bswap32(*prop);
            uart_puts("[Kernel] DETECTED Timer Frequency: ");
            uart_putd(CLOCK_FREQ);
            uart_puts(" Hz\n");
        }
    }
}