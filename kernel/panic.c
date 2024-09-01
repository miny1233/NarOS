#include <nar/panic.h>
#include <nar/printk.h>
#include <device/tty.h>

_Noreturn void panic(char* error)
{
    asm("cli");
    //tty_clear();
    printk("!!! PANIC !!!\n %s",error);
    asm("hlt");
    while(1);
}

void assert_(const char* func,int LINE,char* FILE,char* msg)
{
    char buf[128];
    sprintf(buf,"Error: %s\nFile: %s In Function: %s \nAt Line %d %s",msg,FILE,func,LINE);
    panic(buf);
}