//
// Created by 谢子南 on 2023/9/5.
//

#ifndef NAROS_BITMAP_H
#define NAROS_BITMAP_H

#include <type.h>

u8 bitmap_get(const u8* begin,size_t bit);
void bitmap_set(u8* begin,size_t bit,u8 value);

#endif //NAROS_BITMAP_H
