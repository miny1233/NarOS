//
// Created by 谢子南 on 2023/6/21.
//

#ifndef NAROS_MEM_H
#define NAROS_MEM_H

#define MEMORY_BASE 0x100000    // 1M
#define PAGE_SIZE 0x1000        // 4k Page
#define MEMORY_SIZE 0x1EE0000   // 由于引导程序的设计，导致无法读取可用内存页

void memory_init();
void* get_page();
void put_page(void* addr);

#endif //NAROS_MEM_H
