#include <nar/dev.h>
#include <string.h>
#include <stdio.h>

dev_t dev = -1;

void printk(const char* fmt, ...){  
   static char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);
    if(dev == -1)dev = device_find(DEV_TTY,0)->dev;
    device_write(dev,buf,0,0,0);
}

