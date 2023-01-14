;要是不支持Intel汇编就气死了
.extern main ;声明内核入口
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

;载入主函数(内核启动)
call main

is_started:
mov edx,0x64
cmp eax,edx
jnz is_started

;内核退出
xor bx,bx
mov eax,message
mov cx,0x00

;eax是第一个参数，传入一个字符串指针
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
db "NarOS Stop"
dw 0x00