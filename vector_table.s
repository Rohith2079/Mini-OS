.section .vectors, "ax"
.align 11  // 2048-byte alignment for ARMv8
.global _vector_table
_vector_table:
    // Current EL with SP0
    .align 7  // 128-byte alignment for each vector
    b sync_el1_handler     // Synchronous
    .align 7
    b irq_el1_handler      // IRQ/vIRQ
    .align 7
    b fiq_el1_handler      // FIQ/vFIQ
    .align 7
    b serror_el1_handler   // SError/vSError

    // Current EL with SPx
    .align 7
    b sync_el1_handler     // Synchronous
    .align 7
    b irq_el1_handler      // IRQ/vIRQ
    .align 7
    b fiq_el1_handler      // FIQ/vFIQ
    .align 7
    b serror_el1_handler   // SError/vSError

    // Lower EL using AArch64
    .align 7
    b sync_el1_handler     // Synchronous
    .align 7
    b irq_el1_handler      // IRQ/vIRQ
    .align 7
    b fiq_el1_handler      // FIQ/vFIQ
    .align 7
    b serror_el1_handler   // SError/vSError

    // Lower EL using AArch32
    .align 7
    b sync_el1_handler     // Synchronous
    .align 7
    b irq_el1_handler      // IRQ/vIRQ
    .align 7
    b fiq_el1_handler      // FIQ/vFIQ
    .align 7
    b serror_el1_handler   // SError/vSError