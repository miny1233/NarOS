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