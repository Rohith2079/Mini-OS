#include "user.h"

extern char __stack_top[];

__attribute__((noreturn)) void exit(void) {
    for (;;);
}

void putchar(char ch) {
    /* TODO */
}

__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    __asm__ __volatile__(
        "mov sp, %[stack_top] \n"
        "bl main           \n"
        "bl exit           \n"
        :: [stack_top] "r" (__stack_top)
    );
}
