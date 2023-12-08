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

void* get_cr3();
void set_cr3(void* pde);

struct mm_struct{
    // vma
    void* pte; // 页目录地址
    // data segment
    void* start_brk; //data段起点
    void* brk;       // data段终点

    u8 *kernel_stack; //陷入内核态时的堆栈
};

int copy_pte_to_child(struct mm_struct* father,struct mm_struct* child);

#endif //NAROS_MEM_H
