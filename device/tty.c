#include<device/tty.h>
#include<device/io.h>
#include<type.h>
#include<memory.h>
#include<nar/printk.h>

#define CRT_ADDR_LINE 0x3d4 //CRT地址线
#define CRT_DATA_LINE 0x3d5 //CRT数据线
#define CRT_CURSOR_H 0xe    //Cursor高位
#define CRT_CURSOR_L 0xf    //Cursor低位

u32 high;
u32 width;

static inline void syc_cursor()
{
    u16 pos = high * 80 + width;
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

typedef struct{
    char ch;
    char flag;
}__attribute__((packed)) pix;

void tty_write(const char* str){
   pix (*screen)[80] = (void*)Videos_Mem_Start;
   while(*str != '\0')
   {
    if(*str < '!' || *str > '~') //控制字符
    {
        switch (*str) {
        case '\n':
            high++;
            width=0;
            break;
        case ' ':
            width++;
            break;
        case 8:
            if(width==0)break;
            screen[high][--width].ch = ' ';
            break;
        default:
            printk("%d",*str);
        }
    }else {
        screen[high][width++].ch = *str;
    }
    str++;
    if(width >= 80) //过长
    {
        high++;
        width = 0;
    }
    if(high >= 25) //超过宽度
    {
        for(int i=1;i < 25;i++)
        {
            memcpy(screen[i-1],screen[i],sizeof(pix) * 80);
            high = 24;
            width = 0;
        }
        for(int i=0;i<80;i++)screen[24][i].ch = ' ';
    }
    syc_cursor();
   }
}
void tty_clear(){
    tty_init();
}