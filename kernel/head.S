#define ASM_FILE        1
#include <nar/multiboot.h>
#define STACK_SIZE  0x4000
.global _init
#define AOUT_KLUDGE MULTIBOOT_AOUT_KLUDGE
#define MULTIBOOT_HEADER_FLAGS                  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
#一个小坑，在gnu汇编中calling C 的函数要加下划线
#声明内核入口
.global start,_start

.section .text
start:
_start:
        jmp     multiboot_entry     #GRUB要求4字节对齐 因此只能用跳转保证多重引导头是对齐的

        /* Align 32 bits boundary. */
        .align  4

        /* Multiboot header. */
multiboot_header:
        /* magic */
        .long   MULTIBOOT_HEADER_MAGIC
        /* flags */
        .long   MULTIBOOT_HEADER_FLAGS
        /* checksum */
        .long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
#ifndef __ELF__
        /* header_addr */
        .long   multiboot_header
        /* load_addr */
        .long   _start
        /* load_end_addr */
        .long   _edata
        /* bss_end_addr */
        .long   _end
        /* entry_addr */
        .long   multiboot_entry
#else /* ! __ELF__ */
        .long   0
        .long   0
        .long   0
        .long   0
        .long   0
#endif
legacy:
 movl $0x10,%eax
 movl %eax,%ds
 movl %eax,%es
 movl %eax,%fs
 movl %eax,%gs
 movl %eax,%ss #初始化段寄存器
 movl $0x90000,%esp # 超过0x90000似乎会撞到一些硬件的地址 使得内核工作不正常
 
 movl %esp,%ebp # 初始化栈段,防止下面的call指令产生的eip数据冲掉内核

 xorl %eax,%eax
#检测A20地址开启
cheak_a20:
 inc %eax
 movl %eax,(0x000000)
 cmpl (0x100000),%eax
 jz cheak_a20

multiboot_entry:
 /* Set Stack */
 movl $(stack + STACK_SIZE), %esp
 /* Reset EFLAGS. */
 pushl   $0
 popf

 /* Push the pointer to the Multiboot information structure. */
 pushl   %ebx
/* Push the magic value. */
 pushl   %eax
# 载入主函数(内核启动)
 call init

is_started:
 cmp $0,%eax
 jnz is_started

# 开始负责idle进程
idle:
 hlt
 jmp idle

/* Our stack area. */
.comm   stack, STACK_SIZE
