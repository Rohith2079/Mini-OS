.global sync_el1_handler
sync_el1_handler:
    // Save trap frame
    sub sp, sp, #288              // Make space for trap frame
    stp x0, x1, [sp, #16 * 0]    // Save x0, x1
    stp x2, x3, [sp, #16 * 1]    // Save x2, x3
    stp x4, x5, [sp, #16 * 2]    // Save x4, x5
    stp x6, x7, [sp, #16 * 3]    // Save x6, x7
    stp x8, x9, [sp, #16 * 4]    // Save x8, x9
    stp x10, x11, [sp, #16 * 5]    // Save x8, x9
    stp x12, x13, [sp, #16 * 6]    // Save x8, x9
    stp x14, x15, [sp, #16 * 7]    // Save x8, x9
    stp x16, x17, [sp, #16 * 8]    // Save x8, x9
    stp x18, x19, [sp, #16 * 9]    // Save x8, x9
    stp x20, x21, [sp, #16 * 10]    // Save x8, x9
    stp x22, x23, [sp, #16 * 11]    // Save x8, x9
    stp x24, x25, [sp, #16 * 12]    // Save x8, x9
    stp x26, x27, [sp, #16 * 13]    // Save x8, x9
    stp x28, x29, [sp, #16 * 14]    // Save x8, x9
    str x30, [sp, #16 * 15]
    mrs x21, elr_el1             // Get exception return address
    mrs x22, spsr_el1            // Get saved program status
    stp x21, x22, [sp, #16 * 16] // Save elr and spsr
    mov x0, sp                   // Pass trap frame pointer as argument
    bl handle_trap               // Call C handler
    
    // Restore state and return
    ldp x21, x22, [sp, #16 * 16]
    msr elr_el1, x21
    msr spsr_el1, x22
    ldp x0, x1, [sp, #16 * 0]
    ldp x2, x3, [sp, #16 * 1]
    ldp x4, x5, [sp, #16 * 2]
    ldp x6, x7, [sp, #16 * 3]
    ldp x8, x9, [sp, #16 * 4]
    ldp x10, x11, [sp, #16 * 5]    // Save x8, x9
    ldp x12, x13, [sp, #16 * 6]    // Save x8, x9
    ldp x14, x15, [sp, #16 * 7]    // Save x8, x9
    ldp x16, x17, [sp, #16 * 8]    // Save x8, x9
    ldp x18, x19, [sp, #16 * 9]    // Save x8, x9
    ldp x20, x21, [sp, #16 * 10]    // Save x8, x9
    ldp x22, x23, [sp, #16 * 11]    // Save x8, x9
    ldp x24, x25, [sp, #16 * 12]    // Save x8, x9
    ldp x26, x27, [sp, #16 * 13]    // Save x8, x9
    ldp x28, x29, [sp, #16 * 14]    // Save x8, x9
    ldr x30, [sp, #16 * 15]
    add sp, sp, #288
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
