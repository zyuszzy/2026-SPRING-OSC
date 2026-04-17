# include "uart.h"
# include "type.h"
# include "string.h"
# include "fdt.h"
# include "initrd.h"

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