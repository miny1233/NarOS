mov ax,0x9000
mov ds,ax
;显示进入保护模式
mov ax,0xb800
mov es,ax
xor bx,bx
mov ax,message
mov cx,0x00

put:
push bx
mov bx,ax
mov byte dl,[bx]
cmp cx,[bx]
jz put_end
pop bx
mov byte [es:bx],dl
inc word bx
mov byte [es:bx],0x30
inc word ax
inc word bx
jmp put
put_end:
;准备进入保护模式
cli          ;禁止所有中断
;移动内核模块到内存绝对0处

;移动完毕

;载入段描述符
;源操作数指定一个 6 字节的内存位置，其中包含中断描述符表的基地址和限制。(引用自Intel)
lidt [idt_48]
lgdt [gdt_48]

;启动A20地址线
call test_8042
mov al,0xd1
out 0x64,al    
call test_8042
mov al,0xdf
out 0x60,al
call test_8042 ;缓冲器空，A20地址线已启动

;暂时不设置8259A芯片

;进入保护模式，设置PE位
mov eax,cr0
or eax,0x1
mov cr0,eax ;Intel的建议方法
jmp dword 0b1000:0x00 ;启动内核

;测试8042状态寄存器，等待输入缓冲为空时，进行写命令
test_8042:
  in al,0x64
  test al,0x2 ;检查输入缓冲器
  jnz test_8042
  ret

;临时GDT表
gdt:
dw 0,0,0,0
;代码段
dw 0x07FF ;段限长
dw 0x0000 ;基地址
dw 0x9A00 ;只读，可执行
dw 0x00C0 ;启用粒度，4kb
;数据段
dw 0x07FF
dw 0x0000
dw 0x9200
dw 0x00C0
;IDT寄存器内容（空表）
idt_48:
dw 0
dw 0,0
;GDT寄存器内容
gdt_48:
dw  0x800  ;表长度
dd  gdt+0x90000  ;0x9000:gdt

message:
db "Move to protected mode ..."
dw 0x0000
