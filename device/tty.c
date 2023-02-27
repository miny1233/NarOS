#include<device/tty.h>
#include<type.h>
#include<memory.h>

#define CRT_ADDR_LINE 0x3d4
#define CRT_DATA_LINE 0x3d5
#define CRT_CURSOR_H 0xe
#define CRT_CURSOR_L 0xf

//io
void outb(u16 des,u8 value);
u8 inb(u16 des);

u32 high;
u32 width;

static inline void syc_cursor()
{
    u16 pos = high * 80 + (width>>1);
    outb(CRT_ADDR_LINE,CRT_CURSOR_L);
    outb(CRT_DATA_LINE,pos);
    outb(CRT_ADDR_LINE,CRT_CURSOR_H);
    outb(CRT_DATA_LINE,pos>>8);
}

void tty_init(){
    high=0;
    width=0;
    u32 v_size = 0;
    char *flag = (void*)Videos_Mem_Start+1; 
    char *cursor = (void*)Videos_Mem_Start;
    while(v_size++<80*25)
    {
        *cursor=0;
        *flag=0x30;
        cursor+=2;
        flag+=2;
    }
}
void tty_write(const char* str){
   char *cursor = (void*)Videos_Mem_Start; //80x25
   while(*str!=0)
   {
    if(high==25)
    {
        high--;
        memcpy((void*)Videos_Mem_Start,(void*)Videos_Mem_Start+160,160*24);
        for(int i=0;i<80;i++)
        {
            *(cursor+2*i + 160 * 24)=0;
        }
    }

    switch (*str) {
     case '\n':
        width=0;
        high++;
        str++;
        break;
     default:
        *(cursor + high*160 + width)=*str++;
         width+=2;
         if(!(width%=160))high++;
    }
    syc_cursor();
   }
}
void tty_clear(){
    tty_init();
}