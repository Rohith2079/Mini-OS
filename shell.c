#include "user.h"

//extern char _binary_shell_bin_start[];
//extern char _binary_shell_bin_size[];
//
//void main(void) {
//    uint8_t *shell_bin = (uint8_t *) _binary_shell_bin_start;
//    printf("shell_bin size = %d\n", (int) _binary_shell_bin_size);
//    printf("shell_bin[0] = %x (%d bytes)\n", shell_bin[0]);
//}

void main(void) {
    //putchar('H');
    printf("Hello World!\n");
    char buf[128];
    int len = readfile("hello.txt", buf, sizeof(buf));
    buf[len] = '\0';
    printf("%s\n", buf);
    writefile("hello.txt", "Hello from shell!\n", len);
    __sync_synchronize();
    __asm__ volatile("dsb sy");
    for(;;);
}
