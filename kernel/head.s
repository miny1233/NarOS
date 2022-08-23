;检查A20地址线开启
[bits 32]
xor eax,eax
cheak_a20:
inc eax
mov dword [0x000000],eax
cmp dword eax,[0x100000]
jz cheak_a20
jmp $
message:
db "A20 Startted"
dw 0x00