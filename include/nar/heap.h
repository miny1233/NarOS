//
// Created by 谢子南 on 24-6-1.
//

#ifndef NAROS_HEAP_H
#define NAROS_HEAP_H

#include "type.h"

void heap_init();
void* kalloc(size_t size);
void kfree(void* ptr);

#endif //NAROS_HEAP_H
