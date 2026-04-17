# include "timer.h"
# include "type.h"
# include "uart.h"
# include "string.h"
# include "fdt.h"

unsigned long CLOCK_FREQ;

void timer_init(){

}

unsigned long get_time(){
    unsigned long n;
    asm volatile("rdtime %0" : "=r"(n));
    return n;
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