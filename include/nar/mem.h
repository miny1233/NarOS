//
// Created by 谢子南 on 2023/6/21.
//

#ifndef NAROS_MEM_H
#define NAROS_MEM_H

#define PAGE_SIZE 0x1000        // 4k Page
#define BITMAP_SIZE ((1 * 1024 * 1024) / 8) // 128KB

#include <type.h>

void memory_init();

void* get_cr3();
void set_cr3(void* pde);

void* alloc_page(int page);

struct vm_area_struct{
  void* start;
  void* end;
  uint32_t flags;

  struct vm_area_struct* next;
};

//memory structure
struct mm_struct{
    void* pte; // 页目录地址
    struct vm_area_struct* mmap; // vma

    void* kernel_stack_start; //陷入内核态时的栈地址

    char mm_bitmap[BITMAP_SIZE];
};
//int init_mm_struct(struct mm_struct** mm);
int fork_mm_struct(struct mm_struct* child,struct mm_struct* father);

#endif //NAROS_MEM_H
