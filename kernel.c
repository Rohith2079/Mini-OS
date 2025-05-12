#include "kernel.h"
#include "common.h"
#define USER_STACK_TOP (USER_BASE + 0x1000000) // e.g., 2MB above base
#define USER_STACK_SIZE 0x10000 // 64KB
#define UART0_DR     ((volatile uint32_t *)(0x09000000))
#define UART0_FR     ((volatile uint32_t *)(0x09000018))
#define UART_FR_RXFE (1 << 4)  // Receive FIFO empty
#define UART_FR_TXFF (1 << 5)  // Transmit FIFO full
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[];

extern char __free_ram[], __free_ram_end[];

extern char __kernel_base[];

extern char _binary_shell_bin_start[], _binary_shell_bin_size[];

long uboot_call(long x0, char ch, long x2, long x7) {
    register const char *x1 __asm__("x1") = &ch;
    register long r_x0 __asm__("x0") = x0;
    register long r_x2 __asm__("x2") = x2;
    register long r_x7 __asm__("x7") = x7;
    register long ret __asm__("x0");

    __asm__ __volatile__("hlt 0xf000"
                         : "=r"(ret)
                         : "r"(r_x0), "r"(x1), "r"(r_x2), "r"(r_x7)
                         : "memory");
    return ret;
}

 void putchar(char ch) {
 	uboot_call(0x03, ch, 1, 64);
 }

 signed char getchar(){
     // long ch = uboot_call(0x07, 0, 0, 64);
     // return ch;
     signed char ch;
     long param[3];
     param[0] = 0;          // Handle 0 = stdin
     param[1] = (long)&ch;  // Buffer to store input
     param[2] = 1;          // Number of bytes to read

     // Call semihosting SYS_READ (0x06)
     //long ret = uboot_call(0x07, 0, (long)param, 64);
     long ret = uboot_call(0x03, 0, 0, 64);

     // If ret == 0, one character was read successfully
     // If ret != 0, EOF or error (return -1 or similar)
     return (ret == 0) ? ch : -1;
 }



__attribute__((naked)) void setup_vect(void)
{
    __asm__ __volatile__(
        "adr X0, _vector_table\n"
        "MSR VBAR_EL1, X0\n"
        "mov sp, %0\n"
        "isb\n"
        "RET\n"
        :
        : "r"(0x47010000)
        :);
}

void init_exceptions(void) {
    uint64_t current_el;
    __asm__ __volatile__("mrs %0, CurrentEL" : "=r"(current_el));
    current_el = current_el >> 2;
    
    if (current_el != 1) {
        printf("Warning: Not in EL1 (Current EL: %d)\n", current_el);
        return;
    }

    __asm__ __volatile__(
        "msr mair_el1, %0\n"
        "msr tcr_el1, %1\n"
        "isb\n"
        :: "r"(MAIR_VALUE), "r"(TCR_VALUE)
        : "memory"
    );

    setup_vect();
    
    // Enable MMU and caches
    uint64_t sctlr;
    __asm__ __volatile__("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= SCTLR_MMU_ENABLE;  // Enable MMU
    sctlr |= SCTLR_SA;   // Enable SP alignment check
    sctlr |= SCTLR_I_CACHE;     // Enable I-cache
    sctlr |= SCTLR_D_CACHE;     // Enable D-cache
    __asm__ __volatile__(
        "msr sctlr_el1, %0\n"
        "isb\n"
        :: "r"(sctlr)
        : "memory"
    );

    printf("Exception handling initialized\n");
}


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

__attribute__((naked)) void user_entry(void) {
    __asm__ __volatile__(
        "msr elr_el1, %0         \n"
        "msr spsr_el1, %1        \n"
        "mrs x1, elr_el1\n"
        "mrs x2, spsr_el1\n"
        "eret                    \n"
        :
        : "r"(USER_BASE), "r"(SPSR_MASK)
    );
}

//void user_entry(void) {
//    PANIC("not yet implemented");
//}

struct process *create_process(const void *image, size_t image_size) {
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

    uint64_t *page_table = (uint64_t *) alloc_pages(1);
    if ((uint64_t)page_table & (PAGE_SIZE - 1))
    {
        PANIC("Page table not aligned");
    }

    // Map kernel pages.
    for (paddr_t paddr = (paddr_t) __kernel_base;
         paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE | AARCH64_PAGE_EXECUTE);
    printf("Kernel pages mapped\n");

    map_page(page_table, VIRTIO_BLK_PADDR, VIRTIO_BLK_PADDR, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE);


    // Map user pages.
    printf("image_size = %x\n", (int) image_size);
    for (uint64_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);

        // Handle the case where the data to be copied is smaller than the
        // page size.
        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;

        // Fill and map the page.
        memcpy((void *) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE);
        //printf("User page mapped: %x -> %x\n", USER_BASE + off, page);
    }
    //map execption stack
    paddr_t exception_stack = alloc_pages(1); // 4KB for exception stack
    map_page(page_table, 0x47000000, exception_stack,
             AARCH64_PAGE_READ | AARCH64_PAGE_WRITE);

    // Map user stack
    uint32_t stack_bottom = USER_STACK_TOP - USER_STACK_SIZE;
    for (uint64_t off = 0; off < USER_STACK_SIZE; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);
        map_page(page_table, stack_bottom + off, page, AARCH64_PAGE_READ | AARCH64_PAGE_WRITE);
    }

    //new

    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint64_t)sp;
    proc->page_table = page_table;

    return proc;
}

void handle_data_abort(uint64_t, uint64_t);


void handle_syscall(struct trap_frame *f) {
    // Our convention: syscall number in x8, argument (char) in x0.
    //printf("Syscall: x8=%x x0=%x\n", f->X8, f->X0);
    uint64_t syscall = f->X8;
    f->elr += 4; // Skip the SVC instruction
    switch (syscall) {
        case 1: { // putchar syscall
            char ch = (char) f->X0;
            //f->elr += 4; // Skip the SVC instruction
            putchar(ch);
            break;
        }
        case 2:
        {
            signed char ch;
            int max_attempts = 1000; // Prevent infinite loops
            int attempts = 0;

            while (attempts < max_attempts)
            {
                ch = getchar();
                if (ch >= 0)
                {
                    f->X0 = (uint64_t)ch; // Return the character
                    return;
                }
                attempts++;
                // Small delay between attempts
                for (int i = 0; i < 1000; i++)
                    __asm__ volatile("nop");
            }

            f->X0 = (uint64_t)-1; // Return -1 if no character available
            break;
        }
        default:
            printf("Unknown syscall: %d\n", syscall);
            PANIC("unknown syscall");
    }
    // Restore the state of the registers
    __asm__ __volatile__(
        "msr elr_el1, %0\n"
        :
        : "r"(f->elr) // Restore the ELR register
        : "memory"
    );
}

void handle_trap(struct trap_frame *f) {
    //printf("Stack pointer at trap entry: %x\n", (uint64_t)&f);
    uint64_t esr, far, elr, sp_el0;
    __asm__ __volatile__(
        "mrs %0, esr_el1\n"
        "mrs %1, far_el1\n"
        "mrs %2, elr_el1\n"
        "mrs %3, sp_el0"
        : "=r"(esr), "=r"(far), "=r"(elr), "=r"(sp_el0));
    
    uint64_t ec = (esr >> 26) & 0x3F;
    uint64_t iss = esr & 0x1FFFFFF;

    //printf("Trap: EC=0x%x ISS=0x%x ELR=0x%x SP_EL0=0x%x\n", 
    //       ec, iss, elr, sp_el0);

    uint64_t ttbr0;
    __asm__ __volatile__("mrs %0, ttbr0_el1" : "=r"(ttbr0));
    //printf("TTBR0_EL1=0x%x\n", ttbr0);

    switch (ec) {
        case 0x15:  // SVC instruction execution in AArch64
            handle_syscall(f);
            //PANIC("SVC trap");
            break;
        case 0x20:  // Instruction abort from lower EL
        case 0x21:  // Instruction abort from current EL
        case 0x24:  // Data abort from lower EL
        case 0x25:  // Data abort from current EL
            printf("Data/Instruction Abort:\n");
            printf("  ESR_EL1: 0x%x (EC: 0x%x, ISS: 0x%x)\n", esr, ec, iss);
            printf("  FAR_EL1: 0x%x\n", far);
            printf("  ELR_EL1: 0x%x\n", elr);
            handle_data_abort(esr, far);
            break;
        default:
            printf("Unexpected trap: EC=%x ISS=%x ELR=%x\n", ec, iss, elr);
            PANIC("unexpected trap");
    }
}

uint32_t virtio_reg_read32(uint64_t offset) {
    return *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset));
}

uint64_t virtio_reg_read64(uint64_t offset) {
    return *((volatile uint64_t *) (VIRTIO_BLK_PADDR + offset));
}

void virtio_reg_write32(uint64_t offset, uint32_t value) {
    *((volatile uint32_t *) (VIRTIO_BLK_PADDR + offset)) = value;
}

void virtio_reg_fetch_and_or32(uint64_t offset, uint32_t value) {
    virtio_reg_write32(offset, virtio_reg_read32(offset) | value);
}


struct virtio_virtq *blk_request_vq;
struct virtio_blk_req *blk_req;
paddr_t blk_req_paddr;
unsigned blk_capacity;

struct virtio_virtq *virtq_init(unsigned index) {
    printf("Initializing virtqueue %d\n", index);
    
    // Select the queue
    virtio_reg_write32(VIRTIO_REG_QUEUE_SEL, index);
    __sync_synchronize();
    
    uint32_t max_queue_size = virtio_reg_read32(VIRTIO_REG_QUEUE_NUM_MAX);
    printf("Max queue size: %d\n", max_queue_size);
    
    if (max_queue_size == 0 || max_queue_size < VIRTQ_ENTRY_NUM) {
        printf("virtio: invalid queue size %d\n", max_queue_size);
        return NULL;
    }

    // Allocate and zero queue
    paddr_t virtq_paddr = alloc_pages(align_up(sizeof(struct virtio_virtq), PAGE_SIZE) / PAGE_SIZE);
    struct virtio_virtq *vq = (struct virtio_virtq *) virtq_paddr;
    memset(vq, 0, sizeof(struct virtio_virtq));

    // Set queue size
    virtio_reg_write32(VIRTIO_REG_QUEUE_NUM, VIRTQ_ENTRY_NUM);
    
    // Initialize queue fields
    vq->queue_index = index;
    vq->next_avail = 0;
    vq->last_used_index = 0;
    
    __sync_synchronize();

    // Write queue physical address
    virtio_reg_write32(VIRTIO_REG_QUEUE_PFN, virtq_paddr >> 12);
    __sync_synchronize();

    printf("virtqueue %d initialized at paddr 0x%lx\n", index, virtq_paddr);
    return vq;
}


void virtio_blk_init(void) {
    printf("Initializing virtio block device...\n");
    
    // Reset device
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 0);
    __sync_synchronize();
    
    // Set ACKNOWLEDGE status bit
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, VIRTIO_STATUS_ACK);
    
    // Set DRIVER status bit
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS, 
                      virtio_reg_read32(VIRTIO_REG_DEVICE_STATUS) | VIRTIO_STATUS_DRIVER);
    
    // Set FEATURES_OK status bit
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS,
                      virtio_reg_read32(VIRTIO_REG_DEVICE_STATUS) | VIRTIO_STATUS_FEAT_OK);
    
    // Initialize queue
    blk_request_vq = virtq_init(0);
    if (!blk_request_vq) {
        PANIC("Failed to initialize virtqueue");
    }

    // Set DRIVER_OK status bit
    virtio_reg_write32(VIRTIO_REG_DEVICE_STATUS,
                      virtio_reg_read32(VIRTIO_REG_DEVICE_STATUS) | VIRTIO_STATUS_DRIVER_OK);

    // Get device capacity
    blk_capacity = virtio_reg_read64(VIRTIO_REG_DEVICE_CONFIG) * SECTOR_SIZE;
    printf("virtio-blk: capacity is %d bytes\n", blk_capacity);

    // Allocate request buffer
    blk_req_paddr = alloc_pages(align_up(sizeof(*blk_req), PAGE_SIZE) / PAGE_SIZE);
    blk_req = (struct virtio_blk_req *) blk_req_paddr;
    memset(blk_req, 0, sizeof(*blk_req));
}



// of the head descriptor of the new request.
void virtq_kick(struct virtio_virtq *vq, int desc_index) {
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    vq->avail.index++;
    __sync_synchronize();
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);
    vq->last_used_index++;
}

// Returns whether there are requests being processed by the device.
bool virtq_is_busy(struct virtio_virtq *vq) {
    return vq->last_used_index != vq->queue_index;
}

// Update read_write_disk function
void read_write_disk(void *buf, unsigned sector, int is_write) {
    printf("Starting %s at sector %d\n", is_write ? "write" : "read", sector);
    
    struct virtio_virtq *vq = blk_request_vq;
    uint16_t desc_index = vq->next_avail;
    
    // Setup request header
    blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    blk_req->reserved = 0;
    blk_req->sector = sector;
    blk_req->status = 255; // Set to non-zero
    
    if (is_write) {
        memcpy(blk_req->data, buf, SECTOR_SIZE);
    }

    // Setup descriptor chain
    // Header descriptor
    vq->descs[desc_index].addr = blk_req_paddr;
    vq->descs[desc_index].len = 16;  // type + reserved + sector
    vq->descs[desc_index].flags = VIRTQ_DESC_F_NEXT;
    vq->descs[desc_index].next = (desc_index + 1) % VIRTQ_ENTRY_NUM;

    // Data descriptor
    vq->descs[desc_index + 1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
    vq->descs[desc_index + 1].len = SECTOR_SIZE;
    vq->descs[desc_index + 1].flags = VIRTQ_DESC_F_NEXT;
    if (!is_write) {
        vq->descs[desc_index + 1].flags |= VIRTQ_DESC_F_WRITE;
    }
    vq->descs[desc_index + 1].next = (desc_index + 2) % VIRTQ_ENTRY_NUM;

    // Status descriptor
    vq->descs[desc_index + 2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
    vq->descs[desc_index + 2].len = 1;
    vq->descs[desc_index + 2].flags = VIRTQ_DESC_F_WRITE;
    vq->descs[desc_index + 2].next = 0;

    __sync_synchronize();

    // Add to available ring
    vq->avail.ring[vq->avail.index % VIRTQ_ENTRY_NUM] = desc_index;
    __sync_synchronize();
    vq->avail.index++;
    
    // Save current used index
    uint16_t used_idx = vq->used.index;
    
    // Notify device
    virtio_reg_write32(VIRTIO_REG_QUEUE_NOTIFY, vq->queue_index);

    printf("Waiting for completion (used_idx=%d)...\n", used_idx);

    // Wait for completion
    while (vq->used.index == used_idx) {
        __asm__ volatile("yield");
    }

    // Check status
    if (blk_req->status != 0) {
        printf("virtio: request failed with status %d\n", blk_req->status);
        return;
    }

    if (!is_write) {
        memcpy(buf, blk_req->data, SECTOR_SIZE);
        printf("Read data: %.32s\n", (char*)buf);
    }

    // Update next available descriptor
    vq->next_avail = (desc_index + 3) % VIRTQ_ENTRY_NUM;
}

// void read_write_disk(void *buf, unsigned sector, int is_write) {
//     if (sector >= blk_capacity / SECTOR_SIZE) {
//         printf("virtio: tried to read/write sector=%d, but capacity is %d\n",
//               sector, blk_capacity / SECTOR_SIZE);
//         return;
//     }

//     // For 1024 bytes, we need to handle 2 sectors
//     for (int i = 0; i < 2; i++) {
//         // Construct the request for each 512-byte sector
//         blk_req->sector = sector + i;
//         blk_req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
//         if (is_write)
//             memcpy(blk_req->data, buf + (i * SECTOR_SIZE), SECTOR_SIZE);

//         struct virtio_virtq *vq = blk_request_vq;
//         vq->descs[0].addr = blk_req_paddr;
//         vq->descs[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
//         vq->descs[0].flags = VIRTQ_DESC_F_NEXT;
//         vq->descs[0].next = 1;

//         vq->descs[1].addr = blk_req_paddr + offsetof(struct virtio_blk_req, data);
//         vq->descs[1].len = SECTOR_SIZE;
//         vq->descs[1].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
//         vq->descs[1].next = 2;

//         vq->descs[2].addr = blk_req_paddr + offsetof(struct virtio_blk_req, status);
//         vq->descs[2].len = sizeof(uint8_t);
//         vq->descs[2].flags = VIRTQ_DESC_F_WRITE;

//         virtq_kick(vq, 0);

//         while (virtq_is_busy(vq))
//             ;

//         if (blk_req->status != 0) {
//             printf("virtio: warn: failed to read/write sector=%d status=%d\n",
//                    sector + i, blk_req->status);
//             return;
//         }

//         if (!is_write)
//             memcpy(buf + (i * SECTOR_SIZE), blk_req->data, SECTOR_SIZE);
//     }
// }




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
    init_exceptions();
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

    // idle_proc = create_process(NULL, 0); // updated!
    // idle_proc->pid = 0; // idle
    // current_proc = idle_proc;

    // // new!
    // printf("Creating process : user\n");
    // create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);
    // printf("Created process user\n");

    // printf("yielding to user process\n");
	// yield();
    // printf("yield done\n");

    virtio_blk_init(); 
    char buf[SECTOR_SIZE];
    read_write_disk(buf, 0, false /* read from the disk */);
    printf("first sector: %s\n", buf);

    strcpy(buf, "hello from kernel!!!\n");
    read_write_disk(buf, 0, true /* write to the disk */);

	PANIC("booted\n");
	printf("unreachable here\n");

    for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "MOV X2, #0\n"
        "MSR SPSel, X2\n"       // Switch to EL1
		"MOV SP, %[stack_top]\n"
        "BL kernel_main\n"       // Jump to the kernel main function
        :
        : [stack_top] "r" (__stack_top) // Pass the stack top address as %[stack_top]
    );
}
