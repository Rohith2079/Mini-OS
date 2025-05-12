#include "user.h"
#define SYS_PUTCHAR 1
#define SYS_GETCHAR 2


extern char __user_stack_top[];

__attribute__((noreturn)) void exit(void) {
    for (;;);
}

uint64_t syscall(uint64_t sysno, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
    register uint64_t x0 __asm__("x0") = arg0;
    register uint64_t x1 __asm__("x1") = arg1;
    register uint64_t x2 __asm__("x2") = arg2;
    register uint64_t x8 __asm__("x8") = sysno;

    __asm__ __volatile__(
        "svc #0\n"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x8)
        : "memory"
    );

    return x0;
}

int readfile(const char *filename, char *buf, int len) {
    return syscall(SYS_READFILE, (int) filename, (int) buf, len);
    for(;;);
}

int writefile(const char *filename, const char *buf, int len) {
    return syscall(SYS_WRITEFILE, (int) filename, (int) buf, len);
}


void putchar(char ch) {
    syscall(SYS_PUTCHAR, ch, 0, 0);
    return;
}



int getchar(void) {
    return syscall(SYS_GETCHAR, 0, 0, 0);
}



__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    __asm__ __volatile__(
        "mov sp, %[stack_top] \n"
        "bl main           \n"
        "bl exit           \n"
        :: [stack_top] "r" (__user_stack_top)
    );
}
