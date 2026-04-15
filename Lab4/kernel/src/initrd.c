# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"
# include "initrd.h"

void find_initramfs(const void* fdt){
    int chosen_offset = fdt_path_offset(fdt, "/chosen");
    if(chosen_offset >= 0){
        int len;
        uint32_t* start = (uint32_t*)fdt_getprop(fdt, chosen_offset, "linux,initrd-start", &len);
        if(start){
            if (len == 4) {
                initrd_start = (void*)(unsigned long)bswap32(*start);
            } else if (len == 8) {
                uint64_t high = bswap32(start[0]);
                uint64_t low = bswap32(start[1]);
                initrd_start = (void*)(unsigned long)((high << 32) | low);
            }
        }
    }

    uart_puts("[Initrd] Initrd at: ");
    uart_hex(initrd_start);
    uart_puts("\n");
}

void* find_user_program(const void* rd, const char* filename, unsigned int* size){

    
}