#include "../device/io/tty.c"
void printk(const char* str){
  tty_write(str);
}

