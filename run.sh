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
#WORKING COMMAND:

#qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -semihosting -bios /usr/lib/u-boot/qemu_arm64/u-boot.bin -serial mon:stdio --no-reboot -device loader,file=kernel.elf,addr=0x40200000
#qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -semihosting -bios u-boot.bin -serial mon:stdio --no-reboot -device loader,file=kernel.elf,addr=0x40200000

#dd if=lorem.txt of=disk.img bs=512 count=1
(cd disk && tar cf ../disk.tar --format=ustar *.txt)

qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic -semihosting \
    -bios u-boot.bin \
    -serial mon:stdio --no-reboot \
    -device loader,file=kernel.elf,addr=0x40200000 \
    -d unimp,guest_errors,int,cpu_reset -D qemu.log \
    -drive id=drive0,file=disk.tar,format=raw,if=none \
    -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0
    # -drive id=drive0,file=lorem.txt,format=raw,if=none \
    # -drive if=none,format=raw,file=disk.img,id=hd0 \
    # -device virtio-blk-device,drive=hd0,bus=virtio-mmio-bus.0

