.global syscall
.global syscall_List
.text

#用户用来调用中断
syscall:
 int 0x80

#这个地方的syscall是内核中的，用户无法直接访问，只能通过中断
#用户使用这个__syscall是没有意义的
__syscall:
 lea syscall_List(%rip),%rbx
 xor %rax,%rax
 shl $3,%rax
 add %rax,%rbx
 mov (%rbx),%rax
 jmp *%rax #使用jmp可以不动堆栈来启动目的函数，能够减少很多操作
 ret
