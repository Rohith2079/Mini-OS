.global sync_el1_handler
sync_el1_handler:
	mrs x0, ESR_EL1      // Read Exception Syndrome Register (ESR)
    mrs x1, FAR_EL1      // Read Fault Address Register (FAR)
    bl handle_data_abort
    b .                   // Stay here if it returns

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
