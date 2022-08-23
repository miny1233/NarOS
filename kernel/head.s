;检查A20地址线开启
xor eax,eax
cheak_a20:
inc eax
mov [0x000000],eax
cmp eax,[0x100000]
jz cheak_a20
;显示结果
mov ax,0xb800
mov es,ax
xor bx,bx
mov ax,message
mov cx,0x0000
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
message:
db "A20 Startted"
dw 0x00