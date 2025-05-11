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
    for (;;);

//     while (1) {
// prompt:
//         printf("> ");
//         char cmdline[128];
//         for (int i = 0;; i++) {
//             char ch = getchar();
//             putchar(ch);
//             if (ch < 32 && ch != '\n' && ch != '\r') {
//                 continue;
//             }
//             if (i == sizeof(cmdline) - 1) {
//                 printf("command line too long\n");
//                 goto prompt;
//             } else if (ch == '\r') {
//                 printf("\n");
//                 cmdline[i] = '\0';
//                 break;
//             } else {
//                 cmdline[i] = ch;
//             }
//         }

//         if (strcmp(cmdline, "hello") == 0)
//             printf("Hello world from shell!\n");
//         else
//             printf("unknown command: %s\n", cmdline);
//     }
}
