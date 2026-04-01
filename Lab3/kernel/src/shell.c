# include "shell.h"
# include "mm.h"

void test_alloc_1() {
    uart_puts("Testing memory allocation...\n");
    uart_puts("(1-1) --------------------------------------------------\n");
    char *ptr1 = (char *)allocate(4096);
    uart_puts("(1-2) --------------------------------------------------\n");
    char *ptr2 = (char *)allocate(8000);
    uart_puts("(1-3) --------------------------------------------------\n");
    char *ptr3 = (char *)allocate(4096);
    uart_puts("(1-4) --------------------------------------------------\n");
    char *ptr4 = (char *)allocate(4096);

    uart_puts("(1-5) --------------------------------------------------\n");
    free(ptr1);
    uart_puts("(1-6) -------------------------------------------------\n");
    free(ptr2);
    uart_puts("(1-7) --------------------------------------------------\n");
    free(ptr3);
    uart_puts("(1-8) --------------------------------------------------\n");
    free(ptr4);

    /* Test kmalloc */
    uart_puts("Testing dynamic allocator...\n");
    uart_puts("(2-1) --------------------------------------------------\n");
    char *kmem_ptr1 = (char *)allocate(16);
    uart_puts("(2-2) --------------------------------------------------\n");
    char *kmem_ptr2 = (char *)allocate(32);
    uart_puts("(2-3) --------------------------------------------------\n");
    char *kmem_ptr3 = (char *)allocate(64);
    uart_puts("(2-4) --------------------------------------------------\n");
    char *kmem_ptr4 = (char *)allocate(128);

    uart_puts("(2-5) --------------------------------------------------\n");
    free(kmem_ptr1);
    uart_puts("(2-6) --------------------------------------------------\n");
    free(kmem_ptr2);
    uart_puts("(2-7) --------------------------------------------------\n");
    free(kmem_ptr3);
    uart_puts("(2-8) --------------------------------------------------\n");
    free(kmem_ptr4);

    uart_puts("(2-9) --------------------------------------------------\n");
    char *kmem_ptr5 = (char *)allocate(16);
    uart_puts("(2-10) --------------------------------------------------\n");
    char *kmem_ptr6 = (char *)allocate(32);

    uart_puts("(2-11) --------------------------------------------------\n");
    free(kmem_ptr5);
    uart_puts("(2-12) --------------------------------------------------\n");
    free(kmem_ptr6);

    // Test allocate new page if the cache is not enough
    uart_puts("(3-1) --------------------------------------------------\n");
    void *kmem_ptr[102];
    for (int i=0; i<100; i++) {
        kmem_ptr[i] = (char *)allocate(128);
    }
    for (int i=0; i<100; i++) {
        free(kmem_ptr[i]);
    }

    // Test exceeding the maximum size
    char *kmem_ptr7 = (char *)allocate(MEM_SIZE + 1);
    if (kmem_ptr7 == NULL) {
        uart_puts("Allocation failed as expected for size > MEM_SIZE\n");
    }
    else {
        uart_puts("Unexpected allocation success for size > MEM_SIZE\n");
        free(kmem_ptr7);
    }
}

void kernel_shell(){
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
        if(input_index == 0) continue;
        
        if(strcmp(input_buffer, "help") == 0){
            uart_puts("Available commands:\n");
            uart_puts("  test  - run test_alloc_1().\n");
        }else if(strcmp(input_buffer, "test") == 0){
            test_alloc_1();
        }else{
            uart_puts("Unknown command: ");
            uart_puts(input_buffer);
            uart_puts("\nUse help to get commands.\n");
        }
    }
}