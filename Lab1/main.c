extern char uart_getc(void);
extern void uart_putc(char c);
extern void uart_puts(const char* s);
extern void uart_hex(unsigned long h);

#define SBI_EXT_SET_TIMER 0x0
#define SBI_EXT_SHUTDOWN  0x8
#define SBI_EXT_BASE      0x10

enum sbi_ext_base_fid {
    SBI_EXT_BASE_GET_SPEC_VERSION,
    SBI_EXT_BASE_GET_IMP_ID,
    SBI_EXT_BASE_GET_IMP_VERSION,
    SBI_EXT_BASE_PROBE_EXT,
    SBI_EXT_BASE_GET_MVENDORID,
    SBI_EXT_BASE_GET_MARCHID,
    SBI_EXT_BASE_GET_MIMPID,
};

struct sbiret {
    long error;
    long value;
};

struct sbiret sbi_ecall(int ext,
                        int fid,
                        unsigned long arg0,
                        unsigned long arg1,
                        unsigned long arg2,
                        unsigned long arg3,
                        unsigned long arg4,
                        unsigned long arg5) {
    struct sbiret ret;
    register unsigned long a0 asm("a0") = (unsigned long)arg0;
    register unsigned long a1 asm("a1") = (unsigned long)arg1;
    register unsigned long a2 asm("a2") = (unsigned long)arg2;
    register unsigned long a3 asm("a3") = (unsigned long)arg3;
    register unsigned long a4 asm("a4") = (unsigned long)arg4;
    register unsigned long a5 asm("a5") = (unsigned long)arg5;
    register unsigned long a6 asm("a6") = (unsigned long)fid;
    register unsigned long a7 asm("a7") = (unsigned long)ext;
    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");
    ret.error = a0;
    ret.value = a1;
    return ret;
}

int strcmp(const char *s1, const char *s2){
    while (*s1 && (*s1 == *s2)){
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void print_system_info(){
    uart_puts("System information:\n");

    struct sbiret spec = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION, 0, 0, 0, 0, 0, 0);
    struct sbiret impl_id = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_ID, 0, 0, 0, 0, 0, 0);
    struct sbiret impl_ver = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_IMP_VERSION, 0, 0, 0, 0, 0, 0);

    uart_puts("   OpenSBI specification version: ");
    uart_hex(spec.value);
    uart_puts("\n   Implenmentation ID: ");
    uart_hex(impl_id.value);
    uart_puts("\n   Implenmentation version: ");
    uart_hex(impl_ver.value);
    uart_puts("\n");

}

void start_kernel() {
    char input_buffer[64];
    int input_idx = 0;

    uart_puts("\nStarting kernel ...\n");
    uart_puts("===== OSC LAB1 =====\n");

    while (1) {
        uart_puts("opi-rv2> ");
        input_idx = 0;

        // 讀整行指令
        while(1){
            char input_c = uart_getc();
            if(input_c == '\b' || input_c == 127){
                if(input_idx > 0){
                    input_idx--;
                    uart_puts("\b \b"); 
                }
                continue;
            }

            uart_putc(input_c);

            if(input_c == '\n'){
                input_buffer[input_idx] = '\0';
                break;
            }else if(input_idx < 63){
                input_buffer[input_idx++] = input_c;
            }
        }

        // 空白行
        if(input_idx == 0) continue;

        if(strcmp(input_buffer, "hello") == 0){
            uart_puts("Hello world.\n");
        }else if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  help  - show all commands.\n");
            uart_puts("  hello - print Hello world.\n");
            uart_puts("  info  - print system info.\n");
        }else if(strcmp(input_buffer, "info") == 0){
            print_system_info();
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }

    }
}