#include <device/tty.h>
#include <string.h>
#include <stdio.h>
void printk(const char* fmt, ...){  
   static char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);
  tty_write(buf);
}

