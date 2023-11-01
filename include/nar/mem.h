//
// Created by 谢子南 on 2023/6/21.
//

#ifndef NAROS_MEM_H
#define NAROS_MEM_H

#define PAGE_SIZE 0x1000        // 4k Page

#include <type.h>

void memory_init();
void* get_page();
void put_page(void* addr);

u32 get_cr3();
void set_cr3(u32 pde);

#endif //NAROS_MEM_H
