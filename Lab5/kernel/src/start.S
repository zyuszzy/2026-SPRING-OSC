.section ".text.boot"
.global _start
_start:
    la a3, __bss_start
    la a4, __bss_stop
    ble a4, a3, clear_bss_done
clear_bss:
    sd zero, (a3)
    add a3, a3, 8
    blt a3, a4, clear_bss
clear_bss_done:
    la a2, handle_exception
    csrw stvec, a2
    csrw sscratch, zero
    la sp, _end
    tail start_kernel

.global handle_exception
.align 2
handle_exception:
    csrrw sp, sscratch, sp
    bnez sp, save_context
    csrr sp, sscratch

save_context:
    addi sp, sp, -8 * 35
    sd ra,  8 *  0(sp)
    sd gp,  8 *  2(sp)
    sd tp,  8 *  3(sp)
    sd t0,  8 *  4(sp)
    sd t1,  8 *  5(sp)
    sd t2,  8 *  6(sp)
    sd s0,  8 *  7(sp)
    sd s1,  8 *  8(sp)
    sd a0,  8 *  9(sp)
    sd a1,  8 * 10(sp)
    sd a2,  8 * 11(sp)
    sd a3,  8 * 12(sp)
    sd a4,  8 * 13(sp)
    sd a5,  8 * 14(sp)
    sd a6,  8 * 15(sp)
    sd a7,  8 * 16(sp)
    sd s2,  8 * 17(sp)
    sd s3,  8 * 18(sp)
    sd s4,  8 * 19(sp)
    sd s5,  8 * 20(sp)
    sd s6,  8 * 21(sp)
    sd s7,  8 * 22(sp)
    sd s8,  8 * 23(sp)
    sd s9,  8 * 24(sp)
    sd s10, 8 * 25(sp)
    sd s11, 8 * 26(sp)
    sd t3,  8 * 27(sp)
    sd t4,  8 * 28(sp)
    sd t5,  8 * 29(sp)
    sd t6,  8 * 30(sp)
    
    csrr s0, sscratch
    csrr s1, sepc
    csrr s2, sstatus
    csrr s3, scause
    csrr s4, stval
    sd s0,  8 *  1(sp)     /* 進入中斷前的 SP (可能是 User 或 Kernel) */
    sd s1,  8 * 31(sp)
    sd s2,  8 * 32(sp)
    sd s3,  8 * 33(sp)
    sd s4,  8 * 34(sp)

    csrw sscratch, zero    /* 標記目前在核心中 */
    move a0, sp
    call do_trap

.global ret_from_exception
ret_from_exception:
    ld s1,  8 * 31(sp)      /* sepc */
    ld s2,  8 * 32(sp)      /* sstatus */
    csrw sepc, s1
    csrw sstatus, s2

    /* 判斷返回目標是 U-mode 還是 S-mode */
    andi s0, s2, (1 << 8)   /* 檢查 sstatus.SPP */
    bnez s0, 1f             /* 如果 SPP=1 (回到核心)，跳過 sscratch 恢復 */

    /* 情況 A：回到 User Mode，需要把目前的 SP 設回 sscratch 給下次用 */
    addi s0, sp, 8 * 35
    csrw sscratch, s0
1:
    /* 恢復所有通用暫存器 (除了 s0 和 sp) */
    ld ra,  8 *  0(sp)
    ld gp,  8 *  2(sp)
    ld tp,  8 *  3(sp)
    ld t0,  8 *  4(sp)
    ld t1,  8 *  5(sp)
    ld t2,  8 *  6(sp)
    /* 注意：s0 先不要載入，因為我們等一下還需要用它做邏輯判斷 */
    ld s1,  8 *  8(sp)
    ld a0,  8 *  9(sp)
    ld a1,  8 * 10(sp)
    ld a2,  8 * 11(sp)
    ld a3,  8 * 12(sp)
    ld a4,  8 * 13(sp)
    ld a5,  8 * 14(sp)
    ld a6,  8 * 15(sp)
    ld a7,  8 * 16(sp)
    ld s2,  8 * 17(sp)
    ld s3,  8 * 18(sp)
    ld s4,  8 * 19(sp)
    ld s5,  8 * 20(sp)
    ld s6,  8 * 21(sp)
    ld s7,  8 * 22(sp)
    ld s8,  8 * 23(sp)
    ld s9,  8 * 24(sp)
    ld s10, 8 * 25(sp)
    ld s11, 8 * 26(sp)
    ld t3,  8 * 27(sp)
    ld t4,  8 * 28(sp)
    ld t5,  8 * 29(sp)
    ld t6,  8 * 30(sp)

    /* 關鍵分歧：根據返回模式決定如何處理 sp */
    ld s0,  8 * 32(sp)      /* 重新取得 sstatus 內容到 s0 */
    andi s0, s0, (1 << 8)   /* 再次檢查 SPP */
    bnez s0, nested_ret_sp

    /* 正常返回 User Mode */
    ld s0,  8 *  7(sp)      /* 恢復真正的 s0 */
    ld sp,  8 *  1(sp)      /* 從堆疊載入 User SP */
    sret

nested_ret_sp:
    /* 巢狀返回 Kernel Mode */
    ld s0,  8 *  7(sp)      /* 恢復真正的 s0 */
    addi sp, sp, 8 * 35     /* 直接把指標撥回去彈出 Frame，不 ld sp */
    sret        /*Based on sstatus back to status, and return to instruction which sepc pointed*/

.global switch_to
switch_to:
    /* ----- Save current state ----- */
    // a0: current->thread
    sd ra, 0(a0)           
    sd sp, 8(a0) 
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    /* ----- load next state ----- */
    // a1: next->thread
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)
    mv tp, a1
    ret