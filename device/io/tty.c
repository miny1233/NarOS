#include"tty.h"
#include"../../include/type.h"

u32 high;
u32 width;

void tty_init(){
    high=0;
    width =0;
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
         high%=25;
    }      
   }
}
void tty_clear(){
    tty_init();
}