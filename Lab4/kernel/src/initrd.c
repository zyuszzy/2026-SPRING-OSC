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

    uart_puts("[Kernel] Initrd at: ");
    uart_hex(initrd_start);
    uart_puts("\n");
}

void* find_user_program(const void* rd, const char* filename, unsigned int* size){
    struct cpio_t* header = (struct cpio_t*)rd;
    
    while(1){
        // check magic number
        if(strncmp(header->magic, "070701", 6) != 0)    break;

        int current_file_size = hextoi(header->filesize, 8);
        int current_name_size = hextoi(header->namesize, 8);
        char* current_filename = (char*)(header + 1);

        if(strncmp(current_filename, "TRAILER!!!", 10) == 0) break;

        if(strcmp(current_filename, filename) == 0){
            int data_offset = align(sizeof(struct cpio_t) + current_name_size, 4);
            if(size) 
                *size = current_file_size;
            return (void*)((char*)header + data_offset);
        }
        
        // move to next header
        int data_offset = align(sizeof(struct cpio_t) + current_name_size, 4);
        int next_header_offset = data_offset + align(current_file_size, 4);
        header = (struct cpio_t*)((char*)header + next_header_offset);
    }
    return NULL;
}