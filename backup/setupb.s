org 200h
;set video
mov ah,00h
mov al,12H
int 10h
;write   
mov ah,0x0e
mov bx,message 
cmp bx,bx 
put:
mov al,[bx]
int 0x10 
inc bx  
mov dx,0x00
cmp dx,[bx]
jnz put    

jmp $

message:
db "Read Finish"
dw 0x0

times 510-($-$$) db 0
dw 0xaa55
