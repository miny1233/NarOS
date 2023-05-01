#要是不支持Intel汇编就气死了（气死了）
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

 cli
#内核正常退出，关机
 hlt
#下面的代码会触发保护中断，暂时先废弃

#内核退出
 mov message,%eax
 call put

stop:
 hlt

# eax是第一个参数，传入一个字符串指针
put:
 pushl %ebx
 movl %eax,%ebx
 movb (%ebx),%dl
 cmpl $0,(%ebx)
 jz put_end
 popl %ebx
 movb %dl,0xb8000(%ebx)
 incl %ebx
 movb $0x30,0xb8000(%ebx)
 incl %eax
 incl %ebx
 jmp put
put_end:
 ret

message:
.string "NarOS Stop"
