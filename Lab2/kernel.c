#include "uart.h" 
// Token for DTB structure Block
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

// CPIO header，儲存的是16進位ASCII字串
struct cpio_t {
    char magic[6];          
    char ino[8];            
    char mode[8];           
    char uid[8];            
    char gid[8];            
    char nlink[8];          
    char mtime[8];         
    char filesize[8];       
    char devmajor[8];      
    char devminor[8];       
    char rdevmajor[8];      
    char rdevminor[8];      
    char namesize[8];       
    char check[8];  
};
// 官方 FDT header
struct fdt_header {
    uint32_t magic;                 // 0xd00dfeed (Big-Endian)
    uint32_t totalsize;             
    uint32_t off_dt_struct;         
    uint32_t off_dt_strings;        
    uint32_t off_mem_rsvmap;        
    uint32_t version;               
    uint32_t last_comp_version;     
    uint32_t boot_cpuid_phys;       
    uint32_t size_dt_strings;       
    uint32_t size_dt_struct;        
};

// ------------------------------------------------------------------ Fuctions ------------------------------------------------------------- 
int strcmp(const char *s1, const char *s2){
    while(*s1 && (*s1 == *s2)){
        s1++; s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, int n){
    while(n > 0 && *s1 && (*s1 == *s2)){
        s1++;
        s2++;
        n--;
    }
    if(n == 0) 
        return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}
int strlen(const char *s){
    int len = 0;
    while (*s++) 
        len++;
    return len;
}
static inline uint32_t bswap32(uint32_t x) {
    return ((x & 0x000000ff) << 24) |
           ((x & 0x0000ff00) << 8)  |
           ((x & 0x00ff0000) >> 8)  |
           ((x & 0xff000000) >> 24);
}
static inline uint64_t bswap64(uint64_t x) {
    return ((x & 0x00000000000000ffULL) << 56) |
           ((x & 0x000000000000ff00ULL) << 40) |
           ((x & 0x0000000000ff0000ULL) << 24) |
           ((x & 0x00000000ff000000ULL) << 8)  |
           ((x & 0x000000ff00000000ULL) >> 8)  |
           ((x & 0x0000ff0000000000ULL) >> 24) |
           ((x & 0x00ff000000000000ULL) >> 40) |
           ((x & 0xff00000000000000ULL) >> 56);
}
static inline const void* align_up(const void* ptr, size_t align) {
    return (const void*)(((uintptr_t)ptr + align - 1) & ~(align - 1));      // 公式：(addr + 3) & ~3  (假設 align 為 4)
}
static int hextoi(const char* s, int n) {
    int r = 0;
    while (n-- > 0) {
        r = r << 4;
        if (*s >= 'A')
            r += *s++ - 'A' + 10;
        else if (*s >= 0)
            r += *s++ - '0';
    }
    return r;
}
static int align(int n, int byte) {
    return (n + byte - 1) & ~(byte - 1);
}
// -----------------------------------------------------------------------------------------------------------------------------------------


// ----------------------------------------------------------------- Exercise 2 ------------------------------------------------------------
int name_match(const char *node_name, const char *current_path){
    int i = 0;

    while(node_name[i] != '\0' && node_name[i] != '@' && current_path[i] != '/' && current_path[i] != '\0'){
        if(node_name[i] != current_path[i]) 
            return 0;
        i++;
    }

    if( (node_name[i] == '\0' || node_name[i] == '@') && (current_path[i] == '/' || current_path[i] == '\0') ){
        return i;
    }
    return 0;
}

int fdt_path_offset(const void* fdt, const char* path) {

    struct fdt_header* header = (struct fdt_header*)fdt;
    const char* fdt_base = (const char*)fdt;
    const char* p = fdt_base + bswap32(header->off_dt_struct);      // p目前為struture block開頭

    // special case: root node
    if (strcmp(path, "/") == 0) 
        return (int)(p - fdt_base);

    const char* current_path = path;
    
    // 跳過 root node 的 '/'
    if(*current_path == '/')
        current_path++;

    int current_level = -1;     // 當前所在遍歷的 level
    int skip_until_level = -1;  // 此條路不是要找的，要退回到的level

    while(1){
        uint32_t token = bswap32(*(uint32_t*)p);

        // 現在 p 停在 token 前面
        if(token == FDT_BEGIN_NODE){
             const char* node_name = p + 4;
             current_level++;

             // root node，跳過比對
             if(current_level == 0 && *node_name == '\0'){
                p += 4;
                p = (const char*)align_up(p + 1, 4);
                continue;
             }

             // 開始比對
             if(skip_until_level == -1 && *current_path != '\0'){
                int len = name_match(node_name, current_path);
                if(len > 0){
                    current_path += len;
                    if(*current_path == '/')
                        current_path++;
                    
                    // path查找完 代表找到
                    if(*current_path == '\0')
                        return (int)(p - fdt_base);
                }else{                                  // 此條不是要找的
                    skip_until_level = current_level;
                }
             }

             p += 4;
             p = (const char*)align_up(p + strlen(node_name) + 1, 4);
        }else if(token == FDT_END_NODE){
            if(skip_until_level == current_level)
                skip_until_level = -1;
            current_level--;
            p += 4;
        }else if(token == FDT_PROP){
            p += 4;
            uint32_t prop_len = bswap32(*(uint32_t*)p);
            p += 8;     // 跳過 prop_len 和 prop_name_offset
            p = (const char*)align_up(p + prop_len, 4);
        }else if(token == FDT_END){
            break;
        }else{
            p += 4;
        }
    }
    
    return -1;
}

const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp){

    struct fdt_header* header = (struct fdt_header*)fdt;
    const char* fdt_base = (const char*)fdt;
    const char* p = fdt_base + nodeoffset; 
    const char* string_base = fdt_base + bswap32(header->off_dt_strings);

    // 確認目前指標在BEGINNODE
    if(bswap32(*(uint32_t*)p) != FDT_BEGIN_NODE)
        return NULL;

    // 跳過BEGINNODE & node name
    p += 4;
    p = (const char*)align_up(p + strlen(p) + 1, 4);

    // traverse 該 node 的內容
    while(1){
        uint32_t token = bswap32(*(uint32_t*)p);
        
        if(token == FDT_PROP){
            p += 4;
            uint32_t prop_len = bswap32(*(uint32_t*)p);
            uint32_t prop_name_offset = bswap32(*(uint32_t*)(p + 4));
            p += 8;

            // 比對 property 名稱 
            const char* prop_name = string_base + prop_name_offset;
            if(strcmp(prop_name, name) == 0){
                if(lenp)    // 如果使用者有提供存放長度的變數（不是傳 NULL），才把長度寫進去。
                    *lenp = prop_len;
                return (const void*)p;
            }

            p = (const char*)align_up(p + prop_len, 4);
        }else if(token == FDT_NOP){
            p += 4;
        }else{
            break;
        }
    }
    return NULL;
}
// -----------------------------------------------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------ Exercise 3 -----------------------------------------------------------
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
// -----------------------------------------------------------------------------------------------------------------------------------------

void kernel_shell(void* cpio_addr){
    char input_buffer[64];
    int input_index = 0;
    
    // 讀取使用者輸入
    while(1){
        uart_puts("# ");
        input_index = 0;

        // 讀整行指令
        while(1){
            char input_c = uart_getc();
            if(input_c == '\b' || input_c == 127){
                if(input_index > 0){
                    input_index--;
                    uart_puts("\b \b"); 
                }
                continue;
            }

            uart_putc(input_c);

            if(input_c == '\n'){
                input_buffer[input_index] = '\0';
                break;
            }else if(input_index < 63){
                input_buffer[input_index++] = input_c;
            }

        }

        // 空白行
        if(input_index == 0) continue;
        
        if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  ls  - list all files.\n");
            uart_puts("  cat  - show file's content.\n");
        }else if(strcmp(input_buffer, "ls") == 0){
            initrd_list(cpio_addr);
        }else if(strncmp(input_buffer, "cat ", 4) == 0){
            char* target_file = input_buffer + 4;
            initrd_cat(cpio_addr, target_file);
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}
// ----------------------------------------------------------------------------------------------------------------------------------------


void start_kernel(unsigned long hartid, unsigned long dtb_ptr){

    const void* fdt = (const void*)dtb_ptr;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("[Kernel] Error: Invalid DTB Magic Number.\n");


    // ================================== 找 UART address =================================
    int uart_offset = fdt_path_offset(fdt, "/soc/uart");
    if(uart_offset < 0) uart_offset = fdt_path_offset(fdt, "/soc/serial");
    // 找到
    if(uart_offset >= 0){
        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, uart_offset, "reg", &len);

        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);
    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | reg1;
            if (reg0 == 0) detected_base = reg1;    // 處理常見的 32-bit 位址

            UART_BASE = detected_base; 

            // 判斷 STRIDE
            if (UART_BASE == 0x10000000UL) {
                UART_STRIDE = 1;        // QEMU
            } else {
                UART_STRIDE = 4;        // OrangePi
            }

            uart_puts(">>> DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");

        }else{
            uart_puts("[Kernel] Error: 'reg' property not found.\n");
        }
    }
    // ====================================================================================

    uart_puts("Starting Kernel...\n");
    uart_puts("===== OSC LAB2 =====\n");

    // ================================== 找 initrd address ===============================
    void* cpio_addr = 0;
    int chosen_offset = fdt_path_offset(fdt, "/chosen");
    if(chosen_offset >= 0){
        int len;
        uint32_t* start = (uint32_t*)fdt_getprop(fdt, chosen_offset, "linux,initrd-start", &len);
        if(start){
            cpio_addr = (void*)(unsigned long)bswap32(*start);
        }
    }

    uart_puts("[Kernel] Initrd at: ");
    uart_hex((unsigned long)cpio_addr);
    uart_puts("\n");
    // ===================================================================================

    kernel_shell(cpio_addr);
}