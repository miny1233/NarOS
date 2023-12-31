//
// Created by 谢子南 on 2023/9/5.
//
#include <bitmap.h>

u8 bitmap_get(const u8* begin,size_t bit)
{
    size_t var_offset = bit / 8;
    return (begin[var_offset] >> (bit % 8)) & 0x1;
}

void bitmap_set(u8* begin,size_t bit,u8 value)
{
    size_t var_offset = bit / 8;
    begin[var_offset] &= ~(1 << (bit % 8));
    begin[var_offset] |= value << (bit % 8);
}