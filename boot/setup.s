mov ax,0x9000
mov ds,ax
mov fs,ax
mov gs,ax
mov es,ax
mov ss,ax
mov bp,ax

mov ax,3
int 0x10

;跳过显示
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

;启动A20地址线
call test_8042
mov al,0xd1
out 0x64,al    
call test_8042
mov al,0xdf
out 0x60,al
call test_8042 ;缓冲器空，A20地址线已启动

;移动内核模块到内存绝对0处
;读取内核
mov bx,0x02 ;从6扇区开始读(已经忘了为什么在6扇区了) -> 原来LBA模式没开，是CHS模式，怪不得扇区不对
xor esi,esi ;从地址0开始写(由于是连续写所以只需设置一次)

read_kernel:
;这里的调用方式类似cdcel，从右向左入栈
push 0x00;之前都是从6扇区开始
movzx ax,bh
push ax;Intel不支持8位的push
movzx ax,bl
push ax

jmp LBA_Read ;入栈方式有问题，这里目前使用jmp替代call

read_ok:

inc bx
cmp bx,0x32;结束扇区

jnz read_kernel
;读取完毕

;移动完毕

;载入段描述符
;源操作数指定一个 6 字节的内存位置，其中包含中断描述符表的基地址和限制。(引用自Intel)

lidt [fs:idt_48]
lgdt [fs:gdt_48]

;暂时不设置8259A芯片
;进入保护模式，设置PE位

mov eax,cr0
or eax,0x1
mov cr0,eax ;Intel的建议方法

jmp 8:0 ;启动内核

;测试8042状态寄存器，等待输入缓冲为空时，进行写命令
test_8042:
  in al,0x64
  test al,0x2 ;检查输入缓冲器
  jnz test_8042
  ret

;LBA28硬盘读取(由于BUG暂时不能一次性读多个扇区)(这个函数应该是LBA24启动目前的内核足够了)
;.des_mem_addr(si register) .16-23 .8-15 .地址0-7
LBA_Read:
;复制head程序到0x00
mov dx,0x01F2
mov al,0x01 ;读取的扇区数目(目前的问题是即便设置读取2个扇区等读到后面的扇区后也会全是0)
out dx,al

mov dx,0x01F3
pop ax ;0-7 (注意这里的pop只有低8位是有效的，由于Intel不支持8bit的pop)
out dx,al

mov dx,0x01F4
pop ax ;8-15
out dx,al

mov dx,0x01f5
pop ax ;16-23
out dx,al

mov dx,0x01f6
mov al,0xe0 ;LBA MODE (24-27位设置为0)
out dx,al

;read disk
mov dx,0x01f7
mov al,0x20 ;读命令
out dx,al

waits_r:
nop
in al,dx
and al,0x88
cmp al,0x08 ;准备就绪
jnz waits_r

;硬盘控制器接收完毕
;准备复制到内存
mov ax,0x00;Segment设置为0,使用esi寻址，无需再配置段寄存器
mov ds,ax ;段寄存器不能传入立即数
mov cx,256 ;一次读2字节,256次读取完成一个扇区

mov dx,0x01F0
read_t_mem:
in ax,dx
mov [esi],ax ;ds配si si由调用方配置 (这里直接esi，以便访问整个4GB空间，而无需配置段寄存器)
add esi,2
loop read_t_mem
jmp read_ok

;IDT寄存器内容
idt_48:
dw 0
dd 0
;GDT寄存器内容
gdt_48:
dw  0x800  ;表长度
dd  gdt+0x90000  ;0x9000:gdt

message:
db "Loading Kernel ..."
dw 0x0000

;临时GDT表
gdt:
dd 0,0    ;NULL
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
