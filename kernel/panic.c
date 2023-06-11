#include <nar/panic.h>
#include <nar/printk.h>
#include <device/tty.h>

void panic(char* error)
{
    asm("cli");
    tty_clear();
    printk("!!! PANIC !!!\n"
           "A problem has been detected and Nar has been shut down.\n"
           "If this is the first time you've seen this, restart your computer.\n"
           "If this screen appears again, follow these steps:\n"
           "Check to be sure you have adequate disk space.\n"
           "Try changing video adapters.\n"
           "Check with your hardware vendor for any BIOS updates.\n"
           "Disable BIOS memory options such as caching or shadowing.\n"
           "Technical information:\n%s",error);
    asm("hlt");
}

void _assert_(char* func,int LINE,char* FILE,char* msg)
{
    char buf[128];
    sprintf(buf,"Error: %s\nFile: %s In Function: %s \nAt Line %d %s",msg,FILE,func,LINE);
    panic(buf);
}