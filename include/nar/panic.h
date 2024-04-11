#include "printk.h"
#include <stdio.h>
#include <device/tty.h>

#define assert(e) if(!(e))assert_(__func__,__LINE__,__FILE_NAME__,#e)

void assert_(const char* func,int LINE,char* FILE,char* msg);
void panic(char* error);

#define DEBUG

#ifdef DEBUG
    #define LOG(...) printk("["__FILE_NAME__"] "__VA_ARGS__)
#else
    #define LOG(...)
#endif