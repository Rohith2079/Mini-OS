.global sync_el1_handler
sync_el1_handler:
    // Save trap frame
    //mov x11, sp
    sub sp, sp, #272              // 34 x 8 = 272 bytes
    //mov x10, sp
    //loop_here:
    //b loop_here
    stp x0,  x1,  [sp, #8 *  0]
    stp x2,  x3,  [sp, #8 *  2]
    stp x4,  x5,  [sp, #8 *  4]
    stp x6,  x7,  [sp, #8 *  6]
    stp x8,  x9,  [sp, #8 *  8]
    stp x10, x11, [sp, #8 * 10]
    stp x12, x13, [sp, #8 * 12]
    stp x14, x15, [sp, #8 * 14]
    stp x16, x17, [sp, #8 * 16]
    stp x18, x19, [sp, #8 * 18]
    stp x20, x21, [sp, #8 * 20]
    stp x22, x23, [sp, #8 * 22]
    stp x24, x25, [sp, #8 * 24]
    stp x26, x27, [sp, #8 * 26]
    stp x28, x29, [sp, #8 * 28]
    str x30,      [sp, #8 * 30]      // x30
    mrs x21, spsr_el1
    mrs x22, elr_el1
    str x21,      [sp, #8 * 31]      // spsr
    str x22,      [sp, #8 * 32]      // elr

    //loop_here:
    //mov x10, sp
    //b loop_here
    mov x0, sp                       // Argument: pointer to struct trap_frame
    bl handle_trap                  // Call C handler

    // Restore state and return
    ldr x21, [sp, #8 * 32]          // elr
    ldr x22, [sp, #8 * 31]          // spsr
    msr elr_el1, x21
    msr spsr_el1, x22
    ldp x0,  x1,  [sp, #8 *  0]
    ldp x2,  x3,  [sp, #8 *  2]
    ldp x4,  x5,  [sp, #8 *  4]
    ldp x6,  x7,  [sp, #8 *  6]
    ldp x8,  x9,  [sp, #8 *  8]
    ldp x10, x11, [sp, #8 * 10]
    ldp x12, x13, [sp, #8 * 12]
    ldp x14, x15, [sp, #8 * 14]
    ldp x16, x17, [sp, #8 * 16]
    ldp x18, x19, [sp, #8 * 18]
    ldp x20, x21, [sp, #8 * 20]
    ldp x22, x23, [sp, #8 * 22]
    ldp x24, x25, [sp, #8 * 24]
    ldp x26, x27, [sp, #8 * 26]
    ldp x28, x29, [sp, #8 * 28]
    ldr x30,      [sp, #8 * 30]
    add sp, sp, #272
    eret

.global irq_el1_handler
irq_el1_handler:
    bl handle_irq_el1
    b .

.global fiq_el1_handler
fiq_el1_handler:
    bl handle_fiq_el1
    b .

.global serror_el1_handler
serror_el1_handler:
    bl handle_serror_el1
    b .

.global sync_el2_handler
sync_el2_handler:
    bl handle_sync_el2
    b .                   // Stay here if it returns

.global irq_el2_handler
irq_el2_handler:
    bl handle_irq_el2
    b .

.global fiq_el2_handler
fiq_el2_handler:
    bl handle_fiq_el2
    b .

.global serror_el2_handler
serror_el2_handler:
    bl handle_serror_el2
    b .
