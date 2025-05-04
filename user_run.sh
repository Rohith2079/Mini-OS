#!/bin/bash
set -xue

# QEMU file path
QEMU=qemu-system-aarch64

CC=clang  # Ubuntu users: use CC=clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=aarch64-unknown-elf -fno-stack-protector -ffreestanding -nostdlib"

OBJCOPY=/usr/bin/aarch64-linux-gnu-objcopy

# Build the shell (application)
$CC $CFLAGS -Wl,-Tuser.ld -Wl,-Map=shell.map -o shell.elf shell.c user.c common.c
$OBJCOPY --set-section-flags .bss=alloc,contents -O binary shell.elf shell.bin
$OBJCOPY -I binary -O elf64-littleaarch64 -B aarch64 shell.bin shell.bin.o

