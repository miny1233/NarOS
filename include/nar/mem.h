//
// Created by 谢子南 on 2023/6/21.
//

#ifndef NAROS_MEM_H
#define NAROS_MEM_H

#define PAGE_SIZE 0x1000        // 4k Page
#define BITMAP_SIZE ((1 * 1024 * 1024) / 8) // 128KB

#include <type.h>

__attribute__((inline)) static void mfence()
{
    asm volatile("mfence":::"memory");
}

typedef struct page_entry_t
{
    u8 present : 1;  // 在内存中
    u8 write : 1;    // 0 只读 1 可读可写
    u8 user : 1;     // 1 所有人 0 超级用户 DPL < 3
    u8 pwt : 1;      // page write through 1 直写模式，0 回写模式
    u8 pcd : 1;      // page cache disable 禁止该页缓冲
    u8 accessed : 1; // 被访问过，用于统计使用频率
    u8 dirty : 1;    // 脏页，表示该页缓冲被写过
    u8 pat : 1;      // page attribute table 页大小 4K/4M
    u8 global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    u8 ignored : 3;  // 为操作系统保留
    u32 index : 20;  // 页索引
}__attribute__((packed)) page_entry_t;

void memory_init();

void* get_cr3();
void set_cr3(void* pde);

void* get_page();
void put_page(void*);

void* get_paddr(void* vaddr);

void* kbrk(int);

// 为mmap预留
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
    page_entry_t* pde; // 页目录地址
    struct vm_area_struct* mmap; // vma

    char kernel_stack_start[4096]; // 陷入内核态时的栈地址
    //void* kbrk; // 内核态非共享内存堆

    void* brk;  //堆内存起点
    void* sbrk; //堆内存终点

    //u8* pm_bitmap; //物理内存使用位图 （暂时不记录，虽然会内存泄漏）
};

//给任务0的内存描述符初始化
int init_mm_struct(struct mm_struct* mm);
// int fork_mm_struct(struct mm_struct* child,struct mm_struct* father);

// mm结构体的初始化
void init_user_mm_struct(struct mm_struct* mm);
void free_user_mm_struct(struct mm_struct* mm);

// 堆空间的分配
void* sbrk(struct mm_struct* mm,int increase);

/* 透穿
* 复制数据到mm所描述的虚拟内存空间下
* @sou : 必须是在内核空间的数据
* @des : 必须是有效的虚拟地址
* 函数堆栈必须在内核区域内
*/
void* copy_to_mm_space(struct mm_struct *mm,void* des,void* sou,size_t len);

#endif //NAROS_MEM_H
