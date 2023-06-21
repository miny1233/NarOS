#include <nar/mem.h>
#include <type.h>
#include <nar/panic.h>

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

u32 memory_base = MEMORY_BASE;
u32 memory_size = MEMORY_SIZE;
u32 free_page;

#define IDX(addr) ((u32)addr >> 12) // 取页索引
#define PAGE(idx) ((u32)idx << 12)  // 取页启始

u8 page_map[MEMORY_SIZE];
page_entry_t page_table[1024];  // 页表

static u32 get_cr3(){
    asm volatile("movl %cr3,%eax\n");
}
static void set_cr3(u32 pde){
    asm volatile("movl %%eax,%%cr3\n"::"a"(pde));
}
static void enable_page()   // 启用分页
{
    asm volatile(
            "movl %cr0, %eax\n"
            "orl $0x80000000, %eax\n"
            "movl %eax, %cr0\n");
}

// 初始化页表项
static void entry_init(page_entry_t *entry, u32 index)
{
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->index = index;
}

void mapping_init(); // 映射页

void memory_init()
{
    free_page = IDX(memory_size);
    for(size_t index=0;index < memory_size;index++)
        page_map[index] = 0;

    mapping_init();
}

void* get_page()
{
    for(size_t index=0;index < memory_size;index++)
    {
        if(page_map[index] == 0)
        {
            page_map[index] = 1;
            return memory_base + (void*)PAGE(index);
        }
    }
    panic("Out of Memory");
}

void put_page(void* addr)
{
    assert((u32)addr >= memory_base && (u32)addr < memory_base + memory_size);
    size_t index = IDX(addr);
    assert(index < memory_size)
    page_map[index] = 0;
}

void mapping_init()
{
    
}