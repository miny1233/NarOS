#include <type.h>
#include <string.h>
void strcpy(char* des,const char *sou)
{
    while(*sou!=0)
    {
        *des++=*sou++;
    }
    *des=0;
}

size_t strlen(const char* str)
{
    size_t len=0;
    while(*str++!=0)len++;
    return len;
}

char *strchr (const char *string, int ch )
{
    while (*string && *string != (char)ch)
        string++;

    if (*string == (char)ch)
        return((char *)string);
    return NULL;
}