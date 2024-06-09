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

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS)
    {
        lhs++;
        rhs++;
    }
    return *lhs < *rhs ? -1 : *lhs > *rhs;
}

int strncmp(const char *lhs, const char *rhs,int n)
{
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS && n--)
    {
        lhs++;
        rhs++;
    }
    if (n == 0)
        return 0;

    return *lhs < *rhs ? -1 : 1;
}


#define SEPARATOR1 '/'                                       // 目录分隔符 1
#define SEPARATOR2 '\\'                                      // 目录分隔符 2
#define IS_SEPARATOR(c) (c == SEPARATOR1 || c == SEPARATOR2) // 字符是否位目录分隔符

// 获取第一个分隔符
char *strsep(const char *str)
{
    char *ptr = (char *)str;
    while (true)
    {
        if (IS_SEPARATOR(*ptr))
        {
            return ptr;
        }
        if (*ptr++ == 0)
        {
            return NULL;
        }
    }
}

// 获取最后一个分隔符
char *strrsep(const char *str)
{
    char *last = NULL;
    char *ptr = (char *)str;
    while (true)
    {
        if (IS_SEPARATOR(*ptr))
        {
            last = ptr;
        }
        if (*ptr++ == 0)
        {
            return last;
        }
    }
}