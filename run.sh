#!/bin/bash
set -xue

# QEMU file path
QEMU=qemu-system-aarch64

# Path to clang and compiler flags
CC=clang  # Ubuntu users: use CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=aarch64-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

OBJCOPY=/usr/bin/aarch64-linux-gnu-objcopy

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -I binary -O elf64-littleaarch64 -B aarch64 shell.bin shell.bin.o


$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf \
    kernel.c common.c shell.bin.o vect.s vector_table.s vect.c




# Start QEMU
#$QEMU -machine virt -cpu cortex-a57 -m 1024 -bios /usr/lib/u-boot/qemu_arm64/u-boot.bin -nographic -serial mon:stdio --no-reboot \
#					-kernel kernel.elf

#qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -bios /usr/lib/u-boot/qemu_arm64/u-boot.bin -serial mon:stdio --no-reboot -device loader,file=kernel.elf,addr=0x80200000
qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -semihosting -bios /usr/lib/u-boot/qemu_arm64/u-boot.bin -serial mon:stdio --no-reboot -device loader,file=kernel.elf,addr=0x40200000

