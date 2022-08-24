#include"tty.h"
char *cursor;
void tty_init(){
    cursor = (char*)Videos_Mem_Start;
}
void tty_write(char* str,unsigned int len){
   for(unsigned int index=0;index<len;index++){
    *cursor = *(str + index);
    *(cursor+1) = 0x30;
    cursor+=2;
   }
}
void tty_clear(){
    tty_init();
}