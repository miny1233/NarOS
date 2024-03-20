#include <string.h>
#include <stdio.h>
#include <syscall.h>

int printf(const char *fmt, ...)
{
    static char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    int ret;
    _syscall1(0,ret,buf); // sys_logs

    return i;
}