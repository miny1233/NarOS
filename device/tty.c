#include<device/tty.h>
#include<device/io.h>
#include<type.h>
#include<memory.h>
#include<nar/printk.h>
#include <nar/dev.h>

#define CRT_ADDR_LINE 0x3d4 //CRT地址线
#define CRT_DATA_LINE 0x3d5 //CRT数据线
#define CRT_CURSOR_H 0xe    //Cursor高位
#define CRT_CURSOR_L 0xf    //Cursor低位

u32 high;
u32 width;

static inline void sync_cursor()
{
    //return;
    u16 pos = high * 80 + width;
    outb(CRT_ADDR_LINE,CRT_CURSOR_L);
    outb(CRT_DATA_LINE,pos);
    outb(CRT_ADDR_LINE,CRT_CURSOR_H);
    outb(CRT_DATA_LINE,pos>>8);
}
char empty_video[80 * 2 * 25];

typedef struct{
    char ch;
    char flag;
}__attribute__((packed)) pix;

void tty_print(const char* str){
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
                case 9:
                    width+=8;
                    break;
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
                memcpy(screen[i - 1],screen[i], sizeof(pix) * 80);
                high = 24;
                width = 0;
            }
            for(int i=0;i<80;i++)screen[24][i].ch = ' ';
        }
        sync_cursor();
    }
}

void tty_clear(){
    high=0;
    width=0;
    sync_cursor();
    memcpy((void *)Videos_Mem_Start,empty_video,sizeof (empty_video));
}

static int tty_write(void *dev, void *buf, size_t count, idx_t idx, int flags)
{
    tty_print(buf);
    return 0;
}

void tty_init(){
    high=0;
    width=0;
    u32 v_size = 0;
    char *flag = (void*)Videos_Mem_Start + 1;
    char *cursor = (void*)Videos_Mem_Start;
    while(v_size++ < 80 * 25)
    {
        *cursor=0;
        //*flag=0x30;
        *flag=0x07;
        cursor+=2;
        flag+=2;
        memcpy(empty_video,(void*)Videos_Mem_Start,sizeof(empty_video));
    }
    device_install(DEV_CHAR,DEV_TTY,"main screen",
                   NULL,NULL, tty_write);
}