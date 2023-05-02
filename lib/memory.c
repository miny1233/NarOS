#include <memory.h>
void memcpy(char* des,char* sou,u32 len)
{
    for(int i=0;i<len;i++)
    {
        *des++ = *sou++;
    }
}

void memset(void* des,char context,size_t len)
{
    for(size_t i =0;i<len;i++)
    {
        *(char*)des++ = context;
    }
}