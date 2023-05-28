#include <memory.h>
void memcpy(void* des,void* sou,u32 len)
{
    u32 count_32 = len / 32;
    u32 count_8 = len - count_32;
    u32* d32=des,*s32 = sou;
    for(u32 i=0;i < count_32;i++)
    {
        *d32++ = *s32++;    // 32位复制
    }
    u8 *d8 = (void*)d32,*s8 = (void*)s32;
    for(u32 i=0;i < count_8;i++)
    {
        *d8++ = *s8++;      // 8位复制
    }
}

void memset(void* des,char context,size_t len)
{
    for(size_t i =0;i<len;i++)
    {
        *(char*)des++ = context;
    }
}