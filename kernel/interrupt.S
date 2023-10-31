.global interrupt_debug
.global interrupt_hardler_list
.global interrupt_handler_ret_0x20

.text
.macro INTERRUPT_HANDLER vector,sys
interrupt_handler_\vector:
 push %ds
 push %es
 push %fs
 push %gs
 pusha
.if \sys == 0
 pushl $1233
.endif
 push $\vector        # 传递中断号
 mov $\vector,%ebx
 shl $2,%ebx
 mov interrupt_hardler_list(%ebx),%eax
 call *%eax
interrupt_handler_ret_\vector:
 pop %eax  #维护栈平衡 弹出 vector 和 error
 pop %eax
 popa
 pop %gs
 pop %fs
 pop %es
 pop %ds
 iret
.endm

.global default_int_hardler
default_int_hardler:
 pusha
 pushl $0
 call interrupt_debug
 pop %eax
 popa
 iret

INTERRUPT_HANDLER 0x00, 0#  divide by zero
INTERRUPT_HANDLER 0x01, 0#  debug
INTERRUPT_HANDLER 0x02, 0#  non maskable interrupt
INTERRUPT_HANDLER 0x03, 0#  breakpoint

INTERRUPT_HANDLER 0x04, 0#  overflow
INTERRUPT_HANDLER 0x05, 0#  bound range exceeded
INTERRUPT_HANDLER 0x06, 0#  invalid opcode
INTERRUPT_HANDLER 0x07, 0#  device not avilable

INTERRUPT_HANDLER 0x08, 1#  double fault
INTERRUPT_HANDLER 0x09, 0#  coprocessor segment overrun
INTERRUPT_HANDLER 0x0a, 1#  invalid TSS
INTERRUPT_HANDLER 0x0b, 1#  segment not present

INTERRUPT_HANDLER 0x0c, 1#  stack segment fault
INTERRUPT_HANDLER 0x0d, 1#  general protection fault
INTERRUPT_HANDLER 0x0e, 1#  page fault
INTERRUPT_HANDLER 0x0f, 0#  reserved

INTERRUPT_HANDLER 0x10, 0#  x87 floating point exception
INTERRUPT_HANDLER 0x11, 1#  alignment check
INTERRUPT_HANDLER 0x12, 0#  machine check
INTERRUPT_HANDLER 0x13, 0#  SIMD Floating - Point Exception

INTERRUPT_HANDLER 0x14, 0#  Virtualization Exception
INTERRUPT_HANDLER 0x15, 1#  Control Protection Exception
INTERRUPT_HANDLER 0x16, 0#  reserved
INTERRUPT_HANDLER 0x17, 0#  reserved

INTERRUPT_HANDLER 0x18, 0#  reserved
INTERRUPT_HANDLER 0x19, 0#  reserved
INTERRUPT_HANDLER 0x1a, 0#  reserved
INTERRUPT_HANDLER 0x1b, 0#  reserved

INTERRUPT_HANDLER 0x1c, 0#  reserved
INTERRUPT_HANDLER 0x1d, 0#  reserved
INTERRUPT_HANDLER 0x1e, 0#  reserved
INTERRUPT_HANDLER 0x1f, 0#  reserved

INTERRUPT_HANDLER 0x20, 0#  clock 时钟中断
INTERRUPT_HANDLER 0x21, 0#  keyboard 键盘中断
INTERRUPT_HANDLER 0x22, 0
INTERRUPT_HANDLER 0x23, 0#  com2 串口2
INTERRUPT_HANDLER 0x24, 0#  com1 串口1
INTERRUPT_HANDLER 0x25, 0
INTERRUPT_HANDLER 0x26, 0
INTERRUPT_HANDLER 0x27, 0
INTERRUPT_HANDLER 0x28, 0#  rtc 实时时钟
INTERRUPT_HANDLER 0x29, 0
INTERRUPT_HANDLER 0x2a, 0
INTERRUPT_HANDLER 0x2b, 0
INTERRUPT_HANDLER 0x2c, 0
INTERRUPT_HANDLER 0x2d, 0
INTERRUPT_HANDLER 0x2e, 0#  harddisk1 硬盘主通道
INTERRUPT_HANDLER 0x2f, 0#  harddisk2 硬盘从通道


.data
.global handler_entry_table
handler_entry_table:
    .long interrupt_handler_0x00
    .long interrupt_handler_0x01
    .long interrupt_handler_0x02
    .long interrupt_handler_0x03
    .long interrupt_handler_0x04
    .long interrupt_handler_0x05
    .long interrupt_handler_0x06
    .long interrupt_handler_0x07
    .long interrupt_handler_0x08
    .long interrupt_handler_0x09
    .long interrupt_handler_0x0a
    .long interrupt_handler_0x0b
    .long interrupt_handler_0x0c
    .long interrupt_handler_0x0d
    .long interrupt_handler_0x0e
    .long interrupt_handler_0x0f
    .long interrupt_handler_0x10
    .long interrupt_handler_0x11
    .long interrupt_handler_0x12
    .long interrupt_handler_0x13
    .long interrupt_handler_0x14
    .long interrupt_handler_0x15
    .long interrupt_handler_0x16
    .long interrupt_handler_0x17
    .long interrupt_handler_0x18
    .long interrupt_handler_0x19
    .long interrupt_handler_0x1a
    .long interrupt_handler_0x1b
    .long interrupt_handler_0x1c
    .long interrupt_handler_0x1d
    .long interrupt_handler_0x1e
    .long interrupt_handler_0x1f
    .long interrupt_handler_0x20
    .long interrupt_handler_0x21
    .long interrupt_handler_0x22
    .long interrupt_handler_0x23
    .long interrupt_handler_0x24
    .long interrupt_handler_0x25
    .long interrupt_handler_0x26
    .long interrupt_handler_0x27
    .long interrupt_handler_0x28
    .long interrupt_handler_0x29
    .long interrupt_handler_0x2a
    .long interrupt_handler_0x2b
    .long interrupt_handler_0x2c
    .long interrupt_handler_0x2d
    .long interrupt_handler_0x2e
    .long interrupt_handler_0x2f




