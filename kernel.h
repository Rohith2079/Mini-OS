#include "common.h"

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {}                                                           \
    } while (0)

struct trap_frame {
    uint64_t X0;
    uint64_t X1;
    uint64_t X2;
    uint64_t X3;
    uint64_t X4;
    uint64_t X5;
    uint64_t X6;
    uint64_t X7;
    uint64_t X8;
    uint64_t X9;
    uint64_t X10;
    uint64_t X11;
    uint64_t X12;
    uint64_t X13;
    uint64_t X14;
    uint64_t X15;
    uint64_t X16;
    uint64_t X17;
    uint64_t X18;
    uint64_t X19;
    uint64_t X20;
    uint64_t X21;
    uint64_t X22;
    uint64_t X23;
    uint64_t X24;
    uint64_t X25;
    uint64_t X26;
    uint64_t X27;
    uint64_t X28;
    uint64_t X29;
    uint64_t X30;
    uint64_t spsr;
    uint64_t elr; // Exception Link Register
//} __attribute__((packed));
};

#define READ_SYS_REG(reg)                                                          \
    ({                                                                         \
        unsigned long __tmp;                                                   \
        __asm__ __volatile__("MRS %0, " #reg : "=r"(__tmp));                  \
        __tmp;                                                                 \
    })

#define WRITE_CSR(reg, value)                                                  \
    do {                                                                       \
        uint32_t __tmp = (value);                                              \
        __asm__ __volatile__("MSR " #reg ", %0" ::"r"(__tmp));                \
    } while (0)

#define PROCS_MAX 8       // Maximum number of processes

#define PROC_UNUSED   0   // Unused process control structure
#define PROC_RUNNABLE 1   // Runnable process

// AArch64 descriptor types
#define PTE_VALID   (1 << 0)
#define PTE_TABLE   (1 << 1)
#define PTE_BLOCK   (0 << 1)
#define PTE_PAGE    (1 << 1)  // L3 descriptor, or table entry for L1/L2
#define PTE_AF      (1 << 10) // Access Flag (must be 1 if using MMU)

#define PTE_READ    (0 << 6)
#define PTE_WRITE   (1 << 6)
#define PTE_EXECUTE (0 << 54)  // 0 = executable, 1 = UXN

#define PTE_SH_INNER (3 << 8)  // Inner Shareable
#define PTE_AP_RW    (0 << 6)  // RW for EL1
#define PTE_AP_RO    (1 << 6)  // RO for EL1


#define AARCH64_PAGE_READ     PTE_AP_RW | PTE_AF
#define AARCH64_PAGE_WRITE    PTE_AP_RW | PTE_AF
#define AARCH64_PAGE_EXECUTE  (0 << 54)   // UXN = 0
#define AARCH64_PAGE_USER     (1 << 6)    // EL0 access, if needed

//#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_MASK 0xFFFUL
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define TCR_TG0_4KB     (0 << 14)     // 4KB granule
#define TCR_SH0_INNER   (3 << 12)     // Inner shareable
#define TCR_ORGN0_WBWA  (1 << 10)     // Outer WB WA
#define TCR_IRGN0_WBWA  (1 << 8)      // Inner WB WA
#define TCR_T0SZ        (16ULL << 0)  // 48-bit VA -> 64 - 48 = 16

#define TCR_VALUE (TCR_TG0_4KB | TCR_SH0_INNER | TCR_ORGN0_WBWA | TCR_IRGN0_WBWA | TCR_T0SZ)

#define MAIR_IDX_NORMAL 0
#define MAIR_IDX_DEVICE 1

#define MAIR_ATTR_NORMAL 0xFF  // Inner/Outer Write-Back Write-Allocate
#define MAIR_ATTR_DEVICE 0x00  // Strongly-ordered device

#define MAIR_VALUE ((MAIR_ATTR_NORMAL << (8 * MAIR_IDX_NORMAL)) | \
                    (MAIR_ATTR_DEVICE << (8 * MAIR_IDX_DEVICE)))

#define SCTLR_MMU_ENABLE   (1 << 0)
#define SCTLR_I_CACHE      (1 << 12)
#define SCTLR_D_CACHE      (1 << 2)
#define SCTLR_SA           (1 << 3)  // Stack Alignment Check
#define SCTLR_SA0          (1 << 4)
#define SCTLR_SPAN         (1 << 23)
#define SCTLR_DEFAULT      (SCTLR_MMU_ENABLE | SCTLR_I_CACHE | SCTLR_D_CACHE | SCTLR_SA | SCTLR_SA0 | SCTLR_SPAN)

// The base virtual address of an application image. This needs to match the
// starting address defined in `user.ld`.
#define USER_BASE 0x46000000

//#define SPSR_MASK 0x3C0  // 0b0000001111000000

#define SPSR_MASK (0x0ULL | 0x0 << 2)
#define SPSR_MASK 0x4 //EL1t

struct process {
    int pid;             // Process ID
    int state;           // Process state: PROC_UNUSED or PROC_RUNNABLE
    vaddr_t sp;          // Stack pointer
	uint64_t *page_table;
    __attribute__((aligned(16))) uint8_t stack[8192]; // Kernel stack
};


#define SECTOR_SIZE       512
#define VIRTQ_ENTRY_NUM   16
#define VIRTIO_DEVICE_BLK 2
#define VIRTIO_BLK_PADDR  0x0a000000
//#define VIRTIO_BLK_PADDR  0x10001000
#define VIRTIO_REG_MAGIC         0x00
#define VIRTIO_REG_VERSION       0x04
#define VIRTIO_REG_DEVICE_ID     0x08
#define VIRTIO_REG_QUEUE_SEL     0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM     0x38
#define VIRTIO_REG_QUEUE_ALIGN   0x3c
#define VIRTIO_REG_QUEUE_PFN     0x40
#define VIRTIO_REG_QUEUE_READY   0x44
#define VIRTIO_REG_QUEUE_NOTIFY  0x50
#define VIRTIO_REG_DEVICE_STATUS 0x70
#define VIRTIO_REG_DEVICE_CONFIG 0x100
#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEAT_OK   8
#define VIRTQ_DESC_F_NEXT          1
#define VIRTQ_DESC_F_WRITE         2
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

// VirtIO Register Definitions


// VirtQueue Descriptor Flags
#define VIRTQ_DESC_F_INDIRECT 4

// VirtIO Device IDs

// Virtqueue Descriptor area entry.
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

// Virtqueue Available Ring.
struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue Used Ring entry.
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// Virtqueue Used Ring.
struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue.
struct virtio_virtq {
    // Descriptor Table
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];
    // Available Ring
    struct virtq_avail avail;
    // Used Ring (must be page aligned)
    uint8_t padding[PAGE_SIZE - sizeof(struct virtq_desc) * VIRTQ_ENTRY_NUM - 
                   sizeof(struct virtq_avail)];
    struct virtq_used used;
    // Extra fields for driver use
    uint16_t queue_index;
    uint16_t next_avail; // Track next available descriptor
    uint16_t last_used_index;
} __attribute__((packed)) __attribute__((aligned(PAGE_SIZE)));

// Virtio-blk request.
struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
    uint8_t data[512];
    uint8_t status;
} __attribute__((packed));
