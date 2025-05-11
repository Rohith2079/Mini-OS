ENTRY(boot)

SECTIONS {
    . = 0x40200000;
	__kernel_base = .;

    .text :{
        KEEP(*(.text.boot));
        *(.text .text.*);
    }

    .vectors ALIGN(2048) : {
			*(.vectors)
	}

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(16) {
        *(.data .data.*);
    }

    .bss : ALIGN(16) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 128 * 1024; /* 128KB */
    __stack_top = .;

	. = ALIGN(4096);
    __free_ram = .;
    . += 32 * 1024 * 1024; /* 32MB */
    __free_ram_end = .;
}
