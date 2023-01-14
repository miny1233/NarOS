#include"tty.h"
char *cursor;
void tty_init(){
    cursor = (char*)Videos_Mem_Start;
}
void tty_write(char* str,unsigned int len){
   for(unsigned int index=0;index<len;index++){
    *(cursor++) = *(str + index);//字符
    *(cursor++) = 0x30;//属性
   }
}
void tty_clear(){
    tty_init();
}