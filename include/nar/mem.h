//
// Created by 谢子南 on 2023/6/21.
//

#ifndef NAROS_MEM_H
#define NAROS_MEM_H

#define PAGE_SIZE 0x1000        // 4k Page
#define BITMAP_SIZE ((1 * 1024 * 1024) / 8) // 128KB

#define KERNEL_MEM_SPACE (0x1000000ULL) // 16MB 内核专用内存 (内核代码段 与 数据段) 内存分配时绕过这块物理内存

#define KERNEL_VMA_START KERNEL_MEM_SPACE
#define USER_VMA_START (0x40000000ULL) // 用户态 VMA 开始地址 1GB
#define KERNEL_VMA_END USER_VMA_START

#include <type.h>

void memory_init();

void* get_cr3();
void set_cr3(void* pde);

void* alloc_page(int page);

//为mmap预留
struct vm_area_struct{
  u32 start_page;
  u32 end_page;
  uint32_t flags;

  // 红黑树
  struct vm_area_struct* l_next;
  struct vm_area_struct* r_next;
};

//memory structure
struct mm_struct{
    void* pte; // 页目录地址
    struct vm_area_struct* mmap; // vma

    void* kernel_stack_start; //陷入内核态时的栈地址

    void* brk;  //堆内存起点
    void* sbrk; //堆内存终点

    char pm_bitmap[BITMAP_SIZE]; //物理内存使用位图
};
//int init_mm_struct(struct mm_struct** mm);
int fork_mm_struct(struct mm_struct* child,struct mm_struct* father);

#endif //NAROS_MEM_H
