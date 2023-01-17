org 7c00h

;print status
mov ax,0xb800
mov es,ax
xor bx,bx
mov ax,message
mov cx,0x0
put:
push bx
mov bx,ax
mov byte dl,[bx]
pop bx
mov byte [es:bx],dl
inc word bx
mov byte [es:bx],0x30
inc word ax
inc word bx
push bx
mov bx,ax
cmp cx,[bx]
pop bx
jnz put

;初始化硬盘控制器
;载入setup程序复制到0x90000
mov dx,0x01F2
mov al,0x01
out dx,al

mov dx,0x01F3
mov al,0x02
out dx,al
mov dx,0x01F4
xor al,al
out dx,al
mov dx,0x01f5
out dx,al
mov dx,0x01f6
mov al,0xe0
out dx,al ;66代码好像都忘记写了，没想到之前还读取成功了
;read disk
mov dx,0x01f7
mov al,0x20
out dx,al
waits:
nop
in al,dx
and al,0x88
cmp al,0x08
jnz waits
;硬盘控制器接收完毕
;准备复制到内存
mov ax,SetupSegment
mov ds,ax
xor si,si
mov cx,256
mov dx,0x01F0
read:
in ax,dx
mov [si],ax
add si,2
loop read

;跳转到Setup程序
jmp SetupSegment:0x00
hlt

message:
db "Reading from Disk"
dw 0x00

SetupSegment equ 0x9000

times 510-($-$$) db 0
dw 0xaa55
