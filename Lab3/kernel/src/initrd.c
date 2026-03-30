# include "uart.h"
# include "type.h"
# include "string.h"
# include "initrd.h"

void initrd_list(const void* rd) {
    
    struct cpio_t* header = (struct cpio_t*)rd;
    int file_count = 0;

    // 開始讀所有檔案(先算有幾個檔案)
    while(1){
        if(strncmp(header->magic, "070701", 6) != 0){
            uart_puts("Error: Invalid magic number\n");
            break;
        }

        int file_size = hextoi(header->filesize, 8);   
        int name_size = hextoi(header->namesize, 8);
        char* filename = (char*)(header + 1);

        if(strncmp(filename, "TRAILER!!!", 10) == 0)
            break;

        file_count ++;
        
        // 要跳到下一個 header
        int data_offset = align(name_size + sizeof(struct cpio_t), 4);
        int next_header_offset = data_offset + align(file_size, 4);

        // 移到下一個 header
        header = (struct cpio_t*)((char*)header + next_header_offset);
    }

    uart_puts("Total ");
    uart_putd(file_count);
    uart_puts(" files.\n");

    // 開始讀所有檔案並印出
    header = (struct cpio_t*)rd;
    while(1){
        if(strncmp(header->magic, "070701", 6) != 0){
            uart_puts("Error: Invalid magic number\n");
            break;
        }

        int file_size = hextoi(header->filesize, 8);   
        int name_size = hextoi(header->namesize, 8);
        char* filename = (char*)(header + 1);

        if(strncmp(filename, "TRAILER!!!", 10) == 0)
            break;

        uart_putd(file_size);
        uart_puts("      ");
        uart_puts(filename);
        uart_puts("\n");
        
        // 要跳到下一個 header
        int data_offset = align(name_size + sizeof(struct cpio_t), 4);
        int next_header_offset = data_offset + align(file_size, 4);

        // 移到下一個 header
        header = (struct cpio_t*)((char*)header + next_header_offset);
    }
}

void initrd_cat(const void* rd, const char* filename) {
    
    struct cpio_t* header = (struct cpio_t*)rd;

    // 開始讀所有檔案
    while(1){
        if(strncmp(header->magic, "070701", 6) != 0){
            uart_puts("Error: Invalid magic number\n");
            break;
        }

        int current_file_size = hextoi(header->filesize, 8);
        int current_name_size = hextoi(header->namesize, 8);
        char* current_filename = (char*)(header + 1);

        if(strncmp(current_filename, "TRAILER!!!", 10) == 0)
            break;

        // 找到要的檔案
        if(strcmp(current_filename, filename) == 0){
            int data_offset = align(sizeof(struct cpio_t) + current_name_size, 4);
            char* file_data = (char*)header + data_offset;

            // printf("%.*s", current_file_size, file_data);
            for (int i = 0; i < current_file_size; i++) {
                uart_putc(file_data[i]);
            }
            uart_puts("\n");
            return;
        }

        int data_offset = align(sizeof(struct cpio_t) + current_name_size, 4);
        int next_header_offset = data_offset + align(current_file_size, 4);

        header = (struct cpio_t*)((char*)header + next_header_offset);
    }
    uart_puts("initrd_cat: ");
    uart_puts(filename);
    uart_puts(": No such file\n");
}