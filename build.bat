nasm boot/boot.s -o boot/boot.bin
nasm boot/setup.s -o boot/setup.bin
nasm kernel/head.s -o kernel/head.bin
dd if=boot/boot.bin of=build/xsOS/xsOS.vhd bs=512 count=1
dd if=boot/setup.bin of=build/xsOS/xsOS.vhd bs=512 count=1 seek=1
dd if=kernel/head.bin of=build/xsOS/xsOS.vhd count=1 seek=5
PAUSE