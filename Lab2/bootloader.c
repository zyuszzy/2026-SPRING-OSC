#ifdef USE_QEMU
    #define KERNEL_LOAD_ADDR 0x82000000UL
#else
    #define KERNEL_LOAD_ADDR 0x20000000UL
#endif

#include "uart.h"


// Token for DTB structure Block
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009


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
/*static inline uint32_t bswap32(uint32_t x) {
    return __builtin_bswap32(x);   
}
static inline uint64_t bswap64(uint64_t x) {
    return __builtin_bswap64(x);    
}*/
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
// -----------------------------------------------------------------------------------------------------------------------------------------


// ----------------------------------------------------------------- Exerciese 1 -----------------------------------------------------------
// receive kernel through UART and load kernel
void load_kernel_uart(unsigned long hartid, unsigned long dtb_ptr){
    char* kernel_address = (char*)KERNEL_LOAD_ADDR;    // UART
    unsigned int magic = 0;
    unsigned int kernel_size = 0;

    uart_puts("Please run the Python script for loading kernel...\n");

    // check magic
    for(int i=0 ; i <4 ; i++)
        ((char*)&magic)[i] = uart_getc_raw();
    if(magic != 0x544F4F42){
        uart_puts("Error: magic number is not match.\n");
        return;
    }

    // read kernel size
    for(int i=0 ; i<4 ; i++)
        ((char*)&kernel_size)[i] = uart_getc_raw();
    uart_puts("Loading kernel, the Kernel Size: ");
    uart_hex(kernel_size);
    uart_puts("\n");

    // 開始讀kernel
    for(int i=0 ; i<kernel_size ; i++)
        kernel_address[i] = uart_getc_raw();
    uart_puts("Kernel loaded successfully! Jump to kernel ");
    if(KERNEL_LOAD_ADDR == 0x82000000UL)
        uart_puts("0x82000000 ...\n");
    else if(KERNEL_LOAD_ADDR == 0x20000000UL)
        uart_puts("0x20000000 ...\n");

    // function pointer
    void (*kernel_entry)(unsigned long, unsigned long) = (void (*)(unsigned long, unsigned long))KERNEL_LOAD_ADDR;
    kernel_entry(hartid, dtb_ptr);     // jump to kernel
}

// receive user's input
void shell(unsigned long hartid, unsigned long dtb_ptr){
    char input_buffer[64];
    int input_index = 0;

    uart_puts("\nStarting Bootloader Shell ...\n");
    uart_puts("===== OSC LAB2 =====\n");

    while(1){
        uart_puts("bootloader > ");
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
            uart_puts("  help  - show all commands.\n");
            uart_puts("  load  - receive the kernel image over UART.\n");
        }else if(strcmp(input_buffer, "load") == 0){
            load_kernel_uart(hartid, dtb_ptr);
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}
// -----------------------------------------------------------------------------------------------------------------------------------------


// ----------------------------------------------------------------- Exercise 2 ------------------------------------------------------------
int name_match(const char *node_name, const char *current_path){
    int i = 0;

    // 先比對到字串結束、碰到 '/' 或碰到 '@'
    while (current_path[i] != '\0' && current_path[i] != '/' && current_path[i] != '@') {
        if (node_name[i] != current_path[i]) 
            return 0;
        i++;
    }

    // 情況 A: current_path 在這裡結束了 (或是碰到 '/')
    // 此時 node_name 必須也結束，或者是剛好碰到 '@' 
    if (current_path[i] == '\0' || current_path[i] == '/') {
        if (node_name[i] == '\0' || node_name[i] == '@') {
            return i;
        }
        return 0;
    }

    // 情況 B: current_path 指定了位址 (例如 uart@d4017000)
    // 此時 node_name 必須也完全一致
    if (current_path[i] == '@') {
        while (node_name[i] != '\0' && current_path[i] != '\0' && current_path[i] != '/') {
            if (node_name[i] != current_path[i])
                return 0;
            i++;
        }
        // 確認兩邊都結束或 current_path 碰到下一個層級
        if ((node_name[i] == '\0') && (current_path[i] == '\0' || current_path[i] == '/')) {
            return i;
        }
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
            uint32_t prop_len = bswap32(*(uint32_t*)(p + 4));
            uint32_t name_off = bswap32(*(uint32_t*)(p + 8));
            const char* prop_name = string_base + name_off;

            // 比對 property 名稱 
            if(strcmp(prop_name, name) == 0){
                if(lenp) *lenp = prop_len;  // 如果使用者有提供存放長度的變數（不是傳 NULL），才把長度寫進去。
                return (const void*)(p + 12); // 永遠回傳 Token 往後加 12 的位置
            }

            // 沒找到就跳到下一個 token
            p = (const char*)align_up(p + 12 + prop_len, 4);
        }else if(token == FDT_NOP){
            p += 4;
        }else{
            break;
        }
    }
    return NULL;
}
// -----------------------------------------------------------------------------------------------------------------------------------------


void main(unsigned long hartid, unsigned long dtb_ptr){      // a0:hartid a1:dtb_ptr
     
    uart_puts(">>> DTB locate at: "); uart_hex(dtb_ptr); uart_puts("\n");
    
    const void* fdt = (const void*)dtb_ptr;
    struct fdt_header* header = (struct fdt_header*)fdt;
    // check magic number
    if(bswap32(header->magic) != 0xd00dfeed)
        uart_puts("Error: Invalid DTB Magic Number.\n");

    // try to found UART_BASE
    int offset = fdt_path_offset(fdt, "/soc/uart@d4017000");
    if(offset < 0) offset = fdt_path_offset(fdt, "/soc/serial");
    // 找到
    if(offset >= 0){

        // #######debuggung
        const char* actual_name = (const char*)fdt + offset + 4; // BEGIN_NODE token 後面就是名字
        uart_puts("Node Name: ");
        uart_puts(actual_name);
        uart_puts("\n");
        // ############333########

        int len;
        uint32_t* reg_prop = (uint32_t*)fdt_getprop(fdt, offset, "reg", &len);

        if(reg_prop){
            uint32_t reg0 = bswap32(reg_prop[0]);
            uint32_t reg1 = bswap32(reg_prop[1]);

            // ####degugging#####
            uart_hex((unsigned long)reg0);
            uart_puts("\n");
            uart_hex((unsigned long)reg1);
            // ###############
    
            // 組合出最終位址
            unsigned long detected_base = ((unsigned long)reg0 << 32) | (unsigned long)reg1;
            if (reg0 == 0) detected_base = reg1;    // 處理常見的 32-bit 位址

            uart_puts(">>> DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");

            UART_BASE = detected_base;
            __asm__ volatile ("fence rw, rw"); // 確保之前的寫入先完成，後續讀取再開始

            // 判斷 STRIDE
            /*if (UART_BASE == 0x10000000UL) {
                UART_STRIDE = 1;        // QEMU
            } else {
                UART_STRIDE = 4;        // OrangePi
            }*/

            uart_puts(">>> DETECTED UART BASE: ");
            uart_hex(detected_base);
            uart_puts("\n");
        }else{
            uart_puts("Error: 'reg' property not found.\n");
        }
    }else{
        uart_puts("Error: UART Node NOT found in DTB\n");
    }

    shell(hartid, dtb_ptr);

}