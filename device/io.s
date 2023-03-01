.global outb
.global inb

.text

outb:    # outb(u16 des,u8 value)
 push %ebp
 mov %esp,%ebp

 movl 8(%ebp),%edx
 movl 12(%ebp),%eax

 out %al,%dx 
 leave
 ret 
inb:     # inb(u16 des)
 push %ebp
 mov %esp,%ebp
 movl 8(%ebp),%edx
 in %dx,%al
 leave
 ret
