#include <memory.h>
void memcpy(void* des,const void* sou,u32 len)
{
    asm volatile(
            "cld\n"
            "rep movsb"
            ::"D"(des),"S"(sou),"c"(len)
    );
}

void memset(void* des,char context,size_t len)
{
   asm volatile(
           "rep stosb\n"
           ::"D"(des),"a"(context),"c"(len)
           );
}

void memmove(void* des,void* sou,u32 len)
{
    asm volatile(
            "cmp %%esi,%%edi\n"
            "ja copy_backward\n"
            "je non_write\n"
            "cld\n"
            "jmp write\n"
            "copy_backward:\n"
            "lea -1(%%edi,%%ecx),%%edi\n"
            "lea -1(%%esi,%%ecx),%%esi\n"
            "std\n"
            "write:\n"
            "rep movsb\n"
            "non_write:\n"
            "cld\n"
            ::"D"(des),"S"(sou),"c"(len)
            );
}


int memcmp(const void *str1, const void *str2, size_t n)
{
    int ret;
    asm volatile(
            "cld\n"
            "xorl %%eax,%%eax\n"
            "rep cmpsb\n"
            "jz over\n"
            "sbbl %%eax,%%eax\n"
            "orl $1,%%eax\n"
            "over:\n"
            "nop\n"
            :"=a"(ret)
            :"S"(str1),"D"(str2),"c"(n)
            );
    return ret;
}