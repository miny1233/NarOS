@echo off

echo ����boot
nasm boot/boot.s -o build/boot.bin
nasm boot/setup.s -o build/setup.bin

echo ����kernel
set BUILD_FLAG= -m32 -nostartfiles -fno-builtin -nostdinc -fno-pic -fno-pie -nostdlib -fno-stack-protector
set SOURSE=kernel/head.s device/io.s kernel/main.c

gcc %BUILD_FLAG% %SOURSE% -o build/kernel.bin
objcopy -O binary build/kernel.bin

echo ������ɣ�����д��
dd if=build/boot.bin of=build/c.img bs=512 count=1
dd if=build/setup.bin of=build/c.img bs=512 count=1 seek=1
dd if=build/kernel.bin of=build/c.img bs=512 count=50 seek=2

echo �����Ƿ���ڴ��󣬰���Enter������ϵͳ
PAUSE
cd build
bochs -q
PAUSE