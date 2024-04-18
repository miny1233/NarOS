#define ASM_FILE        1
#include <nar/multiboot.h>
#define STACK_SIZE  0x200000
.global _init
#define AOUT_KLUDGE MULTIBOOT_AOUT_KLUDGE
#define MULTIBOOT_HEADER_FLAGS                  MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
#一个小坑，在gnu汇编中calling C 的函数要加下划线
#声明内核入口
.global start,_start
.global apup

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
 # int $0x20
 jmp idle

.global ap_initialize

.code16
apup:
# enable A20
# 通过系统线启动 (危险)
    in $0x92,%al
    or $0x2,%al
    out %al,$0x92

    #mov $0x1f0,%eax
    # gdt48
    #movw $1023,(%eax)
    #movl $1076928,2(%eax)

    #mov $0x200,%eax
    # idt48
    #movw $2047,(%eax)
    #movl $1078112,2(%eax)

    lgdt %fs:0x1f0  # 特殊定义位置
    lidt %fs:0x200

# enable protect mode
    movl %cr0,%eax
    orl $0x1,%eax
    movl %eax,%cr0

    movl $0x10,%eax # 加载数据段寄存器
    movl %eax,%ds
    movl %eax,%es
    movl %eax,%fs
    movl %eax,%gs
    movl %eax,%ss

    ljmp $8,$0x35 # jmp to slp

    nop # 防止CLion的反编译无法识别代码
    nop

.code32

    nop
    nop
slp:
    xorl %eax,%eax
    leal ap_locker(%eax),%ebx
get_locker:
    lock xchg (%ebx),%eax
    test $1,%eax
    jz get_locker

    # 堆栈是共享的
    movl $(0x1000),%esp    # 加载堆栈
    movl %esp,%ebp

    mov $ap_initialize,%eax
    call *%eax

release_locker:
    movl $1,ap_locker
asleep:
    hlt
    jmp asleep

ap_locker:
    .long 1


/* Our stack area. */
.comm   stack, STACK_SIZE
