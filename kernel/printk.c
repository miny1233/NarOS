#include "../device/tty.c"
void printk(const char* str){
  tty_write(str);
}

