nasm boot/boot.s -o build/boot.bin
nasm boot/setup.s -o build/setup.bin
gcc -m32 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector kernel/head.s kernel/main.c -o build/kernel.bin
dd if=build/boot.bin of=build/c.img bs=512 count=1
dd if=build/setup.bin of=build/c.img bs=512 count=1 seek=1
dd if=build/kernel.bin of=build/c.img bs=512 count=10 seek=2
cd build
bochs -q
PAUSE