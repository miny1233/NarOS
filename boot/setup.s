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
hlt

message:
db "Hello_World xsOS"
dw 0x0000
times 510-($-$$) db 0
dw 0xaa55
