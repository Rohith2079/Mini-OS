ENTRY(start)

SECTIONS {
    . = 0x46000000;

    /* machine code */
    .text :{
        KEEP(*(.text.start));
        *(.text .text.*);
    }

    /* read-only data */
    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    /* data with initial values */
    .data : ALIGN(4) {
        *(.data .data.*);
    }

    /* data that should be zero-filled at startup */
    .bss : ALIGN(4) {
        *(.bss .bss.* .sbss .sbss.*);
    }

    . = 0x47000000;
    
    /* Exception stack section */
    .exception_stack : {
        . = ALIGN(16);
        __exception_stack_start = .;
        . += 4096;  /* 4KB exception stack */
        __exception_stack_top = .;
    }

    /* User stack */
    .user_stack : {
        . = ALIGN(16);
        __user_stack_start = .;
        . += 64 * 1024;  /* 64KB user stack */
        __user_stack_top = .;
    }

    ASSERT(. < 0x47f00000, "too large executable")


}
