.extern _init
#一个小坑，在gnu汇编中calling C 的函数要加下划线
#声明内核入口
.global _start

.section .text
_start:
 movl $0b10000,%eax
 movl %eax,%ds
 movl %eax,%es
 movl %eax,%fs
 movl %eax,%gs
 movl %eax,%ss #初始化段寄存器
 movl $0x90000,%esp
 
 movl %esp,%ebp # 初始化栈段,防止下面的call指令产生的eip数据冲掉内核

 xorl %eax,%eax
#检测A20地址开启
cheak_a20:
 inc %eax
 movl %eax,[0x000000]
 cmpl [0x100000],%eax
 jz cheak_a20
 
# 载入主函数(内核启动)
 call init

is_started:
 cmp $0,%eax
 jnz is_started

# 初始化结束
 hlt

