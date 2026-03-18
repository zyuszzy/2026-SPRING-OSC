.section ".text.boot"
.global _start
_start:
    // Set up the stack pointer
    la sp, _end
    
    la t0, __bss_start
    la t1, __bss_stop

// Clear the .bss section
bss_clear:
    bge t0, t1, bss_done
    sb zero, 0(t0)
    addi t0, t0, 1
    j bss_clear

bss_done:    
    tail start_kernel
