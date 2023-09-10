BUILD:=./build
SRC:=.
GCC:=i686-elf-gcc

IMG:=nar.img

CFLAGS:= -m32 					# 32 位的程序
#CFLAGS+= -Ttext 0       		# 代码段从0开始(否则错误的偏移会找不到数据段的内容)
CFLAGS+= -Ttext 0x100000		# GRUB会将系统引导到1M内存处
CFLAGS+= -static       			# Linux下的ld如果没有此参数无法链接
#CFLAGS+= -march=pentium			# pentium 处理器
CFLAGS+= -fno-builtin			# 不需要 gcc 内置函数
CFLAGS+= -nostdinc				# 不需要标准头文件
CFLAGS+= -fno-pic				# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie				# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib				# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -DDEBUG				# 定义DEBUG模式

DEBUG:= -g

INCLUDE:=-I $(SRC)/include 

KERNEL:=$(SRC)/kernel
DEVICE:=$(SRC)/device
LIB:=$(SRC)/lib

SOURCE:=$(wildcard \
		$(KERNEL)/*.S \
		$(DEVICE)/*.s \
		$(KERNEL)/*.c \
		$(DEVICE)/*.c \
		$(LIB)/*.c \
		$(KERNEL)/fs/*.c \
		) #这里有编译顺序，head.s必须第一个编译


all:
	$(GCC) $(DEBUG) $(INCLUDE) $(CFLAGS) $(SOURCE) -o $(BUILD)/nar
	#nasm boot/boot.s -o $(BUILD)/boot.bin
	#nasm boot/setup.s -o $(BUILD)/setup.bin
	#objcopy -O binary $(BUILD)/nar $(BUILD)/kernel.bin
	#dd if=$(BUILD)/boot.bin   of=$(BUILD)/$(IMG) bs=512 count=1 conv=notrunc
	#dd if=$(BUILD)/setup.bin  of=$(BUILD)/$(IMG) bs=512 count=1 seek=1 conv=notrunc
	#dd if=$(BUILD)/kernel.bin of=$(BUILD)/$(IMG) bs=512 count=50 seek=2 conv=notrunc
	hdiutil mount $(BUILD)/nar.img
	cp  $(BUILD)/nar /Volumes/NAR
	hdiutil unmount /Volumes/NAR
.PHONY:all

build:
	$(GCC) $(DEBUG) $(INCLUDE) $(CFLAGS) $(SOURCE) -o $(BUILD)/nar
	nasm boot/boot.s -o $(BUILD)/boot.bin
	nasm boot/setup.s -o $(BUILD)/setup.bin
	objcopy -O binary $(BUILD)/nar $(BUILD)/kernel.bin
.PHONY:build

asm:
	$(GCC) -S $(INCLUDE) $(CFLAGS) $(KERNEL)/task.c -o $(BUILD)/nar.s
.PHONY:asm

clean:
	rm -f $(BUILD)/*.bin


QEMU:= qemu-system-i386 # 虚拟机
QEMU+= -m 128M # 内存
QEMU+= -rtc base=localtime # 设备本地时间
QEMU+= -drive file=$(BUILD)/nar.img,if=ide,index=0,media=disk,format=raw # 主硬盘
QEMU+= -chardev stdio,mux=on,id=com1 # 字符设备 1
QEMU+= -serial chardev:com1 # 串口 1
QEMU+= -cdrom $(BUILD)/grub.iso # grub

QEMU_DISK_BOOT:=-boot d # d:使用CD-ROM c:使用HDD

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGES)
	make
	make clean
	$(QEMU) $(QEMU_DISK_BOOT)

.PHONY: qemud
qemud: $(IMAGES)
	make
	make clean
	$(QEMU) $(QEMU_DEBUG) $(QEMU_DISK_BOOT)