#include "kernel.h"
#include "common.h"
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[];

extern char __free_ram[], __free_ram_end[];

extern char __kernel_base[];

extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

void uboot_call(long x0, char ch, long x2, long x7) {
    register const char *x1 __asm__("x1") = &ch;
    register long r_x0 __asm__("x0") = x0;
    register long r_x2 __asm__("x2") = x2;
    register long r_x7 __asm__("x7") = x7;

    __asm__ __volatile__("hlt 0xf000"
                         :
                         : "r"(r_x0), "r"(x1), "r"(r_x2), "r"(r_x7)
                         : "memory");
}

void putchar(char ch) {
	uboot_call(0x03, ch, 1, 64);
}

//__attribute__((naked))
//void setup_vect(void){
//	__asm__ __volatile__(
//				"LDR X0, =_vector_table\n"
//				"MSR VBAR_EL1, X0\n"
//				"RET\n"
//				:
//				:
//				:
//				);
//}

paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}

void map_page(uint64_t *l0_table, uint64_t vaddr, paddr_t paddr, uint64_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %lx", vaddr);
    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %lx", paddr);

    // L0 index: bits [47:39]
    uint64_t l0_index = (vaddr >> 39) & 0x1FF;
    uint64_t *l1_table;
    if ((l0_table[l0_index] & PTE_VALID) == 0) {
        paddr_t new_table = alloc_pages(1);
        memset((void *)new_table, 0, PAGE_SIZE);
        l0_table[l0_index] = (new_table & ~PAGE_MASK) | PTE_VALID | PTE_TABLE;
    }
    l1_table = (uint64_t *)(l0_table[l0_index] & ~PAGE_MASK);

    // L1 index: bits [38:30]
    uint64_t l1_index = (vaddr >> 30) & 0x1FF;
    uint64_t *l2_table;
    if ((l1_table[l1_index] & PTE_VALID) == 0) {
        paddr_t new_table = alloc_pages(1);
        memset((void *)new_table, 0, PAGE_SIZE);
        l1_table[l1_index] = (new_table & ~PAGE_MASK) | PTE_VALID | PTE_TABLE;
    }
    l2_table = (uint64_t *)(l1_table[l1_index] & ~PAGE_MASK);

    // L2 index: bits [29:21]
    uint64_t l2_index = (vaddr >> 21) & 0x1FF;
    uint64_t *l3_table;
    if ((l2_table[l2_index] & PTE_VALID) == 0) {
        paddr_t new_table = alloc_pages(1);
        memset((void *)new_table, 0, PAGE_SIZE);
        l2_table[l2_index] = (new_table & ~PAGE_MASK) | PTE_VALID | PTE_TABLE;
    }
    l3_table = (uint64_t *)(l2_table[l2_index] & ~PAGE_MASK);

    // L3 index: bits [20:12]
    uint64_t l3_index = (vaddr >> 12) & 0x1FF;
    l3_table[l3_index] = (paddr & ~PAGE_MASK) | flags | PTE_VALID | PTE_PAGE | PTE_AF;

    //printf("Mapping vaddr: %x to paddr: %x with flags: %x\n", vaddr, paddr, flags);
}

__attribute__((naked)) void switch_context(uint64_t *prev_sp,
                                           uint64_t *next_sp) {
    __asm__ __volatile__(
        // Save callee-saved registers (x19–x30) to current stack
        //"stp x19, x20, [sp, #-16*11]!\n"  // Make space and store x19, x20
        "stp x19, x20, [sp, #-96]!\n"  // Make space and store x19, x20
        "stp x21, x22, [sp, #16 * 1]\n"
        "stp x23, x24, [sp, #16 * 2]\n"
        "stp x25, x26, [sp, #16 * 3]\n"
        "stp x27, x28, [sp, #16 * 4]\n"
        "stp x29, x30, [sp, #16 * 5]\n"   // FP (x29), LR (x30)

        "mov x2, sp\n"                    // Move SP to x2 (temporary register)
        "str x2, [x0]\n"                  // Store x2 (SP) to *prev_sp

        // Load SP from *next_sp (a1 = x1)
        "ldr x2, [x1]\n"                  // Load *next_sp into x2
        "mov sp, x2\n"
               
        // Restore callee-saved registers from new stack
        "ldp x19, x20, [sp], #16\n"
        "ldp x21, x22, [sp], #16\n"
        "ldp x23, x24, [sp], #16\n"
        "ldp x25, x26, [sp], #16\n"
        "ldp x27, x28, [sp], #16\n"
        "ldp x29, x30, [sp], #16\n"

        // Return to LR (x30)
        "ret\n"
    );
}

struct process procs[PROCS_MAX];

//struct process *create_process(uint64_t pc) {
//
//
//    // Find an unused process control structure.
//    struct process *proc = NULL;
//    int i;
//    for (i = 0; i < PROCS_MAX; i++) {
//        if (procs[i].state == PROC_UNUSED) {
//            proc = &procs[i];
//            break;
//        }
//    }
//
//
//    if (!proc)
//        PANIC("no free process slots");
//
//    // Initialize the stack pointer to the top of the stack (aligned)
//    //uint64_t *sp = (uint64_t *) &proc->stack[sizeof(proc->stack)];
//	uint64_t *sp = (uint64_t *)((uint64_t)(proc->stack + sizeof(proc->stack)) & ~0xF);
//
//
//    // Push callee-saved registers (x30 down to x19), matching switch_context()
//    *--sp = pc;   // x30 (lr)
//    *--sp = 0;    // x29 (fp)
//    *--sp = 0;    // x28
//    *--sp = 0;    // x27
//    *--sp = 0;    // x26
//    *--sp = 0;    // x25
//    *--sp = 0;    // x24
//    *--sp = 0;    // x23
//    *--sp = 0;    // x22
//    *--sp = 0;    // x21
//    *--sp = 0;    // x20
//    *--sp = 0;    // x19
//
//    // Ensure 16-byte alignment
//    if ((uint64_t)sp % 16 != 0) {
//        *--sp = 0; // padding
//    }
//
//	uint64_t *page_table = (uint64_t*)alloc_pages(1); //L0 table
//	memset(page_table, 0, PAGE_SIZE);
//    for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE) {
//        map_page(page_table, paddr, paddr, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE | AARCH64_PAGE_EXECUTE);
//    }
//
//    proc->pid = i + 1;
//    proc->state = PROC_RUNNABLE;
//    proc->sp = (uint64_t) sp;
//	proc->page_table = page_table;
//    return proc;
//}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

void mmu_enable(uint64_t *page_table) {
    uint64_t page_table_phys = (uint64_t)page_table;

    __asm__ __volatile__ (
        // Invalidate TLBs before enabling MMU
        "dsb ish\n"
        "isb\n"

        // Set MAIR_EL1
        "msr mair_el1, %[mair_val]\n"

        // Set TCR_EL1
        "msr tcr_el1, %[tcr_val]\n"
        "isb\n"

        // Set TTBR0_EL1 with the base physical address of the page table
        "msr ttbr0_el1, %[ttbr0]\n"
        "isb\n"

        // Read SCTLR_EL1, set MMU and caches
        "mrs x0, sctlr_el1\n"
        "orr x0, x0, %[sctlr_flags]\n"
        "msr sctlr_el1, x0\n"
        "isb\n"
        :
        : [ttbr0] "r" (page_table_phys),
          [tcr_val] "r" (TCR_VALUE),
          [mair_val] "r" (MAIR_VALUE),
          [sctlr_flags] "r" (SCTLR_DEFAULT)
        : "x0", "memory"
    );
}


struct process *proc_a;
struct process *proc_b;

struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

void yield(void) {
    // Search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and continue processing
    if (next == current_proc)
        return;

    // Context switch
    struct process *prev = current_proc;
    current_proc = next;

	// Assume this is your task's stack top
	uint64_t next_sp = (uint64_t)&next->stack[sizeof(next->stack)];
	

    uint64_t new_ttbr0 = ((uint64_t)next->page_table); // Must be physical address, aligned
    uint64_t new_stack = (uint64_t)&next->stack[sizeof(next->stack)];
	
	//printf("new_ttbr0 : %x, new_stack: %x\n", new_ttbr0, new_stack);
	//printf("		WORKING 	\n");

    __asm__ __volatile__ (
        "dsb ishst\n"                     // Ensure memory writes complete
        "tlbi vmalle1\n"                 // Invalidate entire TLB for EL1
        "dsb ish\n"
        "isb\n"
        "mov x0, %[ttbr0]\n"         // Load new TTBR0
        "mrs x1, ttbr0_el1\n"      // Read current TTBR0
        "mrs x2, sctlr_el1\n"      // Read current SCTLR
        "msr ttbr0_el1, %[ttbr0]\n"      // Set new page table
        "mrs x3, ttbr0_el1\n"      // Read current TTBR0

        "isb\n"

        "msr tpidr_el1, %[stack]\n"      // Store stack pointer metadata (optional)
        :
        : [ttbr0] "r" (new_ttbr0),
          [stack] "r" (new_stack)
        : "memory"
    );

	//mmu_enable(next->page_table);

	//printf("yield : post volatile\n");
	//printf("Yielding: prev pid=%d sp=%x -> next pid=%d sp=%x\n",
    //   prev->pid, prev->sp, next->pid, next->sp);

    switch_context(&prev->sp, &next->sp);
	//printf("Yielding context switch done: prev pid=%d sp=%x -> next pid=%d sp=%x\n",
    //   prev->pid, prev->sp, next->pid, next->sp);
	//printf("yield : switch_context\n");
}

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        //switch_context(&proc_a->sp, &proc_b->sp);
        //delay();
		yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        //switch_context(&proc_b->sp, &proc_a->sp);
        //delay();
		yield();
    }
}


void user_entry(void) {
    PANIC("not yet implemented");
}

struct process *create_process(const void *image, size_t image_size) {
    /* omitted */
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }


    if (!proc)
        PANIC("no free process slots");

    // Initialize the stack pointer to the top of the stack (aligned)
    //uint64_t *sp = (uint64_t *) &proc->stack[sizeof(proc->stack)];
	uint64_t *sp = (uint64_t *)((uint64_t)(proc->stack + sizeof(proc->stack)) & ~0xF);

    *--sp = (uint64_t) user_entry;   // x30 (lr)
    *--sp = 0;    // x29 (fp)
    *--sp = 0;    // x28
    *--sp = 0;    // x27
    *--sp = 0;    // x26
    *--sp = 0;    // x25
    *--sp = 0;    // x24
    *--sp = 0;    // x23
    *--sp = 0;    // x22
    *--sp = 0;    // x21
    *--sp = 0;    // x20
    *--sp = 0;    // x19

    uint32_t *page_table = (uint32_t *) alloc_pages(1);

    // Map kernel pages.
    for (paddr_t paddr = (paddr_t) __kernel_base;
         paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE | AARCH64_PAGE_EXECUTE);


    // Map user pages.
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);

        // Handle the case where the data to be copied is smaller than the
        // page size.
        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        // Fill and map the page.
        memcpy((void *) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page, AARCH64_PAGE_USER | AARCH64_PAGE_READ | AARCH64_PAGE_WRITE | AARCH64_PAGE_EXECUTE);
    }
}




void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);
	//const char *s = "\n\nHello World!\n";
    //for (int i = 0; s[i] != '\0'; i++) {
    //    putchar(s[i]);
    //}
	//printf("\n\nHello %s\n", "World!");
	//printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);
	//const char *s1 = "cmp s1";
	//const char *s2 = "cmp s2";
	//if (!strcmp(s1, s2))
	//	printf("s1 == s2\n");
	//else
	//	printf("s1 != s2\n");
	//setup_vect();
	//volatile uint32_t *ptr = (uint32_t *)0xDEADBEEF;
	//*ptr = 42;

	//paddr_t paddr0 = alloc_pages(2);
    //paddr_t paddr1 = alloc_pages(1);
    //printf("alloc_pages test: paddr0=%x\n", paddr0);
    //printf("alloc_pages test: paddr1=%x\n", paddr1);

	//idle_proc = create_process((uint64_t) NULL);
    //idle_proc->pid = 0; // idle
    //current_proc = idle_proc;

	//proc_a = create_process((uint64_t) proc_a_entry);
	//proc_b = create_process((uint64_t) proc_b_entry);
	////proc_a_entry();
	//yield();

	idle_proc = create_process(NULL, 0); // updated!
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    // new!
    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

	yield();


	PANIC("booted\n");
	printf("unreachable here\n");

    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
		"MOV SP, %[stack_top]\n"
        "BL kernel_main\n"       // Jump to the kernel main function
        :
        : [stack_top] "r" (__stack_top) // Pass the stack top address as %[stack_top]
    );
}
