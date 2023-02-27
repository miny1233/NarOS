#include <device/tty.h>
void printk(const char* str){
  tty_write(str);
}

