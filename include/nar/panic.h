#include "printk.h"
#include <stdio.h>
#include <device/tty.h>

#define assert(e) if(!(e))_assert_(__func__,__LINE__,__FILE_NAME__,#e)

void _assert_(char* func,int LINE,char* FILE,char* msg);
void panic(char* error);