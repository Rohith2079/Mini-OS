.section .vectors, "ax"
.align 11
.global _vector_table
_vector_table:
    // Exception from EL0 to EL1
    b sync_el1_handler         // Synchronous exception
    b irq_el1_handler          // IRQ interrupt
    b fiq_el1_handler          // FIQ interrupt
    b serror_el1_handler       // SError interrupt

    // Exception from EL1 to EL1
    b sync_el1_handler
    b irq_el1_handler
    b fiq_el1_handler
    b serror_el1_handler

    // Exception from EL0 to EL2
    b sync_el2_handler
    b irq_el2_handler
    b fiq_el2_handler
    b serror_el2_handler

    // Exception from EL1 to EL2
    b sync_el2_handler
    b irq_el2_handler
    b fiq_el2_handler
    b serror_el2_handler

