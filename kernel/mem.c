#include <nar/mem.h>
#include <type.h>
#include <nar/panic.h>
#include <nar/interrupt.h>
#include <memory.h>
#include <nar/multiboot.h>
#include <math.h>

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

u32 memory_base = 0x200000;  //由于GRUB把内核载入到了1M处，为了保证安全至少从2M开始有效
u32 memory_size = 0;
u32 total_page;

#define DIDX(addr) (((u32)(addr) >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((u32)(addr) >> 12) & 0x3ff) // 获取 addr 的页表索引
#define IDX(addr) ((u32)(addr) >> 12) // 取页索引
#define PAGE(idx) ((u32)(idx) << 12)  // 取页启始

u8* page_map;

page_entry_t* page_table;  // 页目录

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

static void mapping_init(); // 映射页

void memory_init()
{
    LOG("memory init\n");
    //内存状态的检测
    LOG("total mem size is %d MB\n",device_info->mem_upper >> 10);
    if (device_info->flags & (1 << 6))
    {
        multiboot_memory_map_t *mmap;

        LOG("mmap_addr = 0x%x, mmap_length = 0x%x\n",
                (unsigned) device_info->mmap_addr, (unsigned) device_info->mmap_length);
        /*
         * 事例程序中提供了这样一段代码
         * https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#kernel_002ec
         * 这个结构是比较离谱的size成员在-4字节处
         */
        for (mmap = (multiboot_memory_map_t *) device_info->mmap_addr;
             (unsigned long) mmap < device_info->mmap_addr + device_info->mmap_length;
             mmap = (multiboot_memory_map_t *) ((unsigned long) mmap
                                                + mmap->size + sizeof (mmap->size))) {
            LOG(" size = 0x%x, base_addr = 0x%x%08x,"
                   " length = 0x%x%08x, type = 0x%x\n",
                   (unsigned) mmap->size,
                   (unsigned) (mmap->addr >> 32),
                   (unsigned) (mmap->addr & 0xffffffff),
                   (unsigned) (mmap->len >> 32),
                   (unsigned) (mmap->len & 0xffffffff),
                   (unsigned) mmap->type);
            if (memory_size < (mmap->len & 0xffffffff) && mmap->type == 0x1) {
                memory_base = (mmap->addr & 0xffffffff);
                memory_size = (mmap->len & 0xffffffff);
            }
        }
    }
    LOG("mem base 0x%X size: 0x%X\n",memory_base,memory_size);
    total_page = IDX(memory_size);
    LOG("total page 0x%x\n",total_page);
    page_map = (void*)memory_base;  //开头的内存分配给内存表
    memory_base += ((total_page / PAGE_SIZE) + 1) * 4096;//表要能记录total_page个内存
    memory_size -= ((total_page / PAGE_SIZE) + 1) * 4096;//那么剩余的内存就减少了
    LOG("can use mem base 0x%X size: 0x%X\n",memory_base,memory_size);
    for(size_t index=0;index < total_page;index++)
        page_map[index] = 0; //设置为全0
    mapping_init();
}

void* get_page()
{
    for(size_t index=0;index < total_page;index++)
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
    assert((u32)addr >= memory_base && (u32)addr < memory_base + memory_size); // 内存必须在可用区域
    size_t index = IDX((u32)addr - memory_base);
    assert(index < total_page); //不能比总页面数大
    page_map[index] = 0;
}

void page_int(u32 vector)   // 缺页中断
{
    panic("Error: Segment Failed\n");
}

static void mapping_init()
{
    interrupt_hardler_register(0x0e, page_int); //注册缺页中断

    page_table = get_page(); // 取一页内存用作页目录
    page_entry_t* pte = get_page(); // 取一页内存用作页表
    memset(page_table,0,PAGE_SIZE); // 全0可以使present为0 便于触发缺页中断
    memset(pte,0,PAGE_SIZE);
    LOG("page_table at 0x%x\n",page_table);

    entry_init(&page_table[0],IDX((u32)pte));   // 页目录0->内核页表
    for(u32 index=0;index < PAGE_SIZE/4; index++)// 映射一张页表 总计4M
    {
        entry_init(&pte[index],index);   // 映射物理内存在原来的位置
    }
    set_cr3((u32)page_table); // cr3指向页目录
    enable_page();
}

void mmap(void* addr,void* vaddr)
{

}