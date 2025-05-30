#include "common.h"
#include "kernel.h"

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

void handle_sync_el1(void) {
    while (1) {
        // Add UART prints or logging here
    }
}

void handle_irq_el1(void) {
    while (1) {
        // Handle IRQ interrupts
    }
}

void handle_fiq_el1(void) {
    while (1) {
        // Handle FIQ interrupts
    }
}

void handle_serror_el1(void) {
    while (1) {
        // Handle system errors
    }
}


void handle_sync_el2(void) {
    while (1) {
        // Add UART prints or logging here
    }
}

void handle_irq_el2(void) {
    while (1) {
        // Handle IRQ interrupts
    }
}

void handle_fiq_el2(void) {
    while (1) {
        // Handle FIQ interrupts
    }
}

void handle_serror_el2(void) {
    while (1) {
        // Handle system errors
    }
}

