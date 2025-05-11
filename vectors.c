#include "kernel.h"

void sync_sp0(void);
void irq_sp0(void);
void fiq_sp0(void);
void serror_sp0(void);
void sync_spx(void);
void irq_spx(void);
void fiq_spx(void);
void serror_spx(void);
void sync_el0(void);
void irq_el0(void);
void fiq_el0(void);
void serror_el0(void);
void sync_el0_32(void);
void irq_el0_32(void);
void fiq_el0_32(void);
void serror_el0_32(void);

void handle_trap(struct trap_frame *f);

// Vector table with handlers for different exception levels
__attribute__((aligned(2048))) // Must be 2048-byte aligned
void (*_vector_table[])(void) = {
    // Current EL with SP0
    sync_sp0,    // Synchronous
    irq_sp0,     // IRQ
    fiq_sp0,     // FIQ 
    serror_sp0,  // System Error

    // Current EL with SPx
    sync_spx,    // Synchronous
    irq_spx,     // IRQ
    fiq_spx,     // FIQ
    serror_spx,  // System Error

    // Lower EL using AArch64
    sync_el0,    // Synchronous
    irq_el0,     // IRQ
    fiq_el0,     // FIQ
    serror_el0,  // System Error

    // Lower EL using AArch32
    sync_el0_32, // Synchronous 
    irq_el0_32,  // IRQ
    fiq_el0_32,  // FIQ
    serror_el0_32// System Error
};

// Handler macros to avoid code duplication
#define HANDLER_ENTRY(name) \
__attribute__((naked)) void name(void) { \
    __asm__ __volatile__( \
        "sub sp, sp, #512\n" \
        "stp x0, x1, [sp, #16 * 0]\n" \
        "stp x2, x3, [sp, #16 * 1]\n" \
        "stp x4, x5, [sp, #16 * 2]\n" \
        "stp x6, x7, [sp, #16 * 3]\n" \
        "stp x8, x9, [sp, #16 * 4]\n" \
        "stp x10, x11, [sp, #16 * 5]\n" \
        "stp x12, x13, [sp, #16 * 6]\n" \
        "stp x14, x15, [sp, #16 * 7]\n" \
        "stp x16, x17, [sp, #16 * 8]\n" \
        "stp x18, x19, [sp, #16 * 9]\n" \
        "stp x20, x21, [sp, #16 * 10]\n" \
        "stp x22, x23, [sp, #16 * 11]\n" \
        "stp x24, x25, [sp, #16 * 12]\n" \
        "stp x26, x27, [sp, #16 * 13]\n" \
        "stp x28, x29, [sp, #16 * 14]\n" \
        "str x30, [sp, #16 * 15]\n" \
        "mrs x1, spsr_el1\n" \
        "mrs x2, elr_el1\n" \
        "stp x1, x2, [sp, #16 * 16]\n" \
        "mov x0, sp\n" \
        "bl handle_trap\n" \
        "ldp x1, x2, [sp, #16 * 16]\n" \
        "msr spsr_el1, x1\n" \
        "msr elr_el1, x2\n" \
        "ldp x0, x1, [sp, #16 * 0]\n" \
        "ldp x2, x3, [sp, #16 * 1]\n" \
        "ldp x4, x5, [sp, #16 * 2]\n" \
        "ldp x6, x7, [sp, #16 * 3]\n" \
        "ldp x8, x9, [sp, #16 * 4]\n" \
        "ldp x10, x11, [sp, #16 * 5]\n" \
        "ldp x12, x13, [sp, #16 * 6]\n" \
        "ldp x14, x15, [sp, #16 * 7]\n" \
        "ldp x16, x17, [sp, #16 * 8]\n" \
        "ldp x18, x19, [sp, #16 * 9]\n" \
        "ldp x20, x21, [sp, #16 * 10]\n" \
        "ldp x22, x23, [sp, #16 * 11]\n" \
        "ldp x24, x25, [sp, #16 * 12]\n" \
        "ldp x26, x27, [sp, #16 * 13]\n" \
        "ldp x28, x29, [sp, #16 * 14]\n" \
        "ldr x30, [sp, #16 * 15]\n" \
        "add sp, sp, #512\n" \
        "eret\n" \
    ); \
}

// Define handlers for each vector table entry
HANDLER_ENTRY(sync_sp0)
HANDLER_ENTRY(irq_sp0)
HANDLER_ENTRY(fiq_sp0)
HANDLER_ENTRY(serror_sp0)
HANDLER_ENTRY(sync_spx)
HANDLER_ENTRY(irq_spx)
HANDLER_ENTRY(fiq_spx)
HANDLER_ENTRY(serror_spx)
HANDLER_ENTRY(sync_el0)
HANDLER_ENTRY(irq_el0)
HANDLER_ENTRY(fiq_el0)
HANDLER_ENTRY(serror_el0)
HANDLER_ENTRY(sync_el0_32)
HANDLER_ENTRY(irq_el0_32)
HANDLER_ENTRY(fiq_el0_32)
HANDLER_ENTRY(serror_el0_32)



void handle_data_abort(uint64_t esr, uint64_t far) {
    // Extract Exception Class (EC) from ESR
    
    uint64_t ec = (esr >> 26) & 0x3F;
    
    // Extract Data Fault Status Code (DFSC) from ESR
    uint64_t dfsc = esr & 0x3F;
    
    // Extract Instruction Specific Syndrome
    uint64_t iss = esr & 0x1FFFFFF;
    
    printf("Data Abort Exception!\n");
    printf("ESR_EL1: 0x%x\n", esr);
    printf("FAR_EL1: 0x%x\n", far);
    printf("EC: 0x%x\n", ec);
    printf("DFSC: 0x%x\n", dfsc);
    printf("ISS: 0x%x\n", iss);
    PANIC("Data abort exception");
    //while(1);
}