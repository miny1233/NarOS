;检查A20地址线开启
[bits 32]
mov ax,0b10000
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax; 初始化段寄存器
mov esp,0x90000

xor eax,eax
cheak_a20:
inc eax
mov dword [0x000000],eax
cmp dword eax,[0x100000]
jz cheak_a20

;复制main程序到0x200
mov dx,0x01F2
mov al,0x01
out dx,al
mov dx,0x01F3
mov al,0x07
out dx,al
mov dx,0x01F4
xor al,al
out dx,al
mov dx,0x01f5
out dx,al
mov dx,0x01f6
mov al,0xe0
;read disk
mov dx,0x01f7
mov al,0x20
out dx,al
waits_r:
nop
in al,dx
and al,0x88
cmp al,0x08
jnz waits_r
;硬盘控制器接收完毕
;准备复制到内存
mov esi,0x200
mov ecx,256
mov dx,0x01F0
read_t_mem:
in ax,dx
mov [esi],ax
add esi,2
loop read_t_mem

call 0x200
is_started:
mov edx,0x64
cmp eax,edx
jnz is_started

;A20启动完成
xor bx,bx
mov eax,message
mov cx,0x00
put:
push ebx
mov ebx,eax
mov byte dl,[ebx]
cmp cx,[ebx]
jz put_end
pop ebx
mov byte [0xb8000+ebx],dl
inc dword ebx
mov byte [0xb8000+ ebx],0x30
inc dword eax
inc dword ebx
jmp put
put_end:
jmp $

message:
db "A20 Started"
dw 0x00