# include "shell.h"
# include "mm.h"

void test_alloc_1() {
    /***************** Case 2 *****************/

uart_puts("\n===== Part 1 =====\n");

void *p1 = allocate(129);
mm_free_lists();
free(p1);
mm_free_lists();

uart_puts("\n=== Part 1 End ===\n");

uart_puts("\n===== Part 2 =====\n");

// Allocate all blocks at order 0, 1, 2 and 3
int NUM_BLOCKS_AT_ORDER_0 = 0;  // Need modified
int NUM_BLOCKS_AT_ORDER_1 = 0;
int NUM_BLOCKS_AT_ORDER_2 = 0;
int NUM_BLOCKS_AT_ORDER_3 = 0;

void *ps0[NUM_BLOCKS_AT_ORDER_0];
void *ps1[NUM_BLOCKS_AT_ORDER_1];
void *ps2[NUM_BLOCKS_AT_ORDER_2];
void *ps3[NUM_BLOCKS_AT_ORDER_3];
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_0; ++i) {
    ps0[i] = allocate(4096);
}
mm_free_lists();
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_1; ++i) {
    ps1[i] = allocate(8192);
}
mm_free_lists();
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_2; ++i) {
    ps2[i] = allocate(16384);
}
mm_free_lists();
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_3; ++i) {
    ps3[i] = allocate(32768);
}
mm_free_lists();

uart_puts("\n-----------\n");

long MAX_BLOCK_SIZE = PAGE_SIZE * (1 << MAX_ORDER);

/* **DO NOT** uncomment this section */
void *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *p2, *p3, *p4, *p5, *p6, *p7;

p1 = allocate(4095);
mm_free_lists();
free(p1);                        // 4095
mm_free_lists();
p1 = allocate(4095);
mm_free_lists();

c1 = allocate(1000);
mm_free_lists();
c2 = allocate(1023);
mm_free_lists();
c3 = allocate(999);
mm_free_lists();
c4 = allocate(1010);
mm_free_lists();
free(c3);                        // 999
mm_free_lists();
c5 = allocate(989);  
mm_free_lists();
c3 = allocate(88);
mm_free_lists();
c6 = allocate(1001);
mm_free_lists();
free(c3);                        // 88
mm_free_lists();
c7 = allocate(2045);
mm_free_lists();
c8 = allocate(1);
mm_free_lists();

p2 = allocate(4096);
mm_free_lists();
free(c8);                        // 1
mm_free_lists();
p3 = allocate(16000);
mm_free_lists();
free(p1);                        // 4095
mm_free_lists();
free(c7);                        // 2045
mm_free_lists();
p4 = allocate(4097);
mm_free_lists();
p5 = allocate(MAX_BLOCK_SIZE + 1);
mm_free_lists();
p6 = allocate(MAX_BLOCK_SIZE);
mm_free_lists();
free(p2);                        // 4096
mm_free_lists();
free(p4);                        // 4097
mm_free_lists();
p7 = allocate(7197);
mm_free_lists();

free(p6);                        // MAX_BLOCK_SIZE
mm_free_lists();
free(p3);                        // 16000
mm_free_lists();
free(p7);                        // 7197
mm_free_lists();
free(c1);                        // 1000
mm_free_lists();
free(c6);                        // 1001
mm_free_lists();
free(c2);                        // 1023
mm_free_lists();
free(c5);                        // 989
mm_free_lists();
free(c4);                        // 1010
mm_free_lists();


uart_puts("\n-----------\n");

// Free all blocks remaining
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_0; ++i) {
    free(ps0[i]);
}
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_1; ++i) {
    free(ps1[i]);
}
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_2; ++i) {
    free(ps2[i]);
}
for (int i = 0; i < NUM_BLOCKS_AT_ORDER_3; ++i) {
    free(ps3[i]);
}

uart_puts("\n=== Part 2 End ===\n");
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