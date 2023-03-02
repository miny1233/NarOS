.global interrupt_process
.global interrupt_debug
.global keyboard_handler

.text
interrupt_process:
 pusha
# call interrupt_debug
 call keyboard_handler
 popa
 iret




