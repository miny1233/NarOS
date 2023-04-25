BUILD:=./build
SRC:=.
GCC:=i686-elf-gcc

CFLAGS:= -m32 					# 32 位的程序
CFLAGS+= -march=pentium			# pentium 处理器
CFLAGS+= -fno-builtin			# 不需要 gcc 内置函数
CFLAGS+= -nostdinc				# 不需要标准头文件
CFLAGS+= -fno-pic				# 不需要位置无关的代码  position independent code
CFLAGS+= -fno-pie				# 不需要位置无关的可执行程序 position independent executable
CFLAGS+= -nostdlib				# 不需要标准库
CFLAGS+= -fno-stack-protector	# 不需要栈保护
CFLAGS+= -Ttext 0       		# 代码段从0开始(否则错误的偏移会找不到数据段的内容)

DEBUG:= -g

INCLUDE:=-I $(SRC)/include 

KERNEL:=$(SRC)/kernel
DEVICE:=$(SRC)/device
LIB:=$(SRC)/lib

SOURCE:=$(wildcard $(KERNEL)/*.c $(DEVICE)/*.c $(KERNEL)/*.s $(DEVICE)/*.s $(LIB)/*.c)


build:
	$(GCC) $(DEBUG) $(INCLUDE) $(CFLAGS) $(SOURCE) -o $(BUILD)/nar
	objcopy -O binary $(BUILD)/nar $(BUILD)/nar.bin
.PHONY:build

clean:
	rm -f $(BUILD)/nar $(BUILD)/nar.bin