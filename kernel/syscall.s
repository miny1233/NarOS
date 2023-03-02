.global syscall
.global syscall_List
.text

#用户用来调用中断
syscall:
 int $0x80
 ret

#这个地方的syscall是内核中的，用户无法直接访问，只能通过中断
#用户使用这个__syscall是没有意义的
__syscall:
 lea (syscall_List),%ebx
 xor %eax,%eax
 shl $2,%eax
 add %eax,%ebx
 mov (%ebx),%eax
 jmp *%eax #使用jmp可以不动堆栈来启动目的函数，能够减少很多操作
 iret
