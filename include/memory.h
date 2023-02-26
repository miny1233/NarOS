#include "../include/type.h"
void memcpy(char* des,char* sou,u32 len)
{
    for(int i=0;i<len;i++)
    {
        *des++ = *sou++;
    }
}