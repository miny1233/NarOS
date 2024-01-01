#include <nar/mem.h>
#include <type.h>
#include <nar/panic.h>
#include <nar/interrupt.h>
#include <memory.h>
#include <nar/multiboot.h>
#include <math.h>
#include <nar/task.h>
#include <bitmap.h>

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
    u8 ignored : 3;  // 为操作系统保留 (Nar中用于记录这个内存是否是当前程序从内存中申请的内存)
    u32 index : 20;  // 页索引
}__attribute__((packed)) page_entry_t;

u32 memory_base = 0;  //由于GRUB把内核载入到了1M处，为了保证安全至少从2M开始有效
u32 memory_size = 0;
u32 total_page;

#define DIDX(addr) (((u32)(addr) >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((u32)(addr) >> 12) & 0x3ff) // 获取 addr 的页表索引
#define IDX(addr) ((u32)(addr) >> 12) // 取页索引
#define PAGE(idx) ((void*)((u32)(idx) << 12))  // 取页启始

#define PTE_SIZE (PAGE_SIZE/sizeof(page_entry_t))

#define KERNEL_MEM_SPACE (0x1000000ULL) // 16MB 内核专用内存 (内核代码段 与 数据段) 内存分配时绕过这块物理内存

#define KERNEL_VMA_START KERNEL_MEM_SPACE
#define USER_VMA_START (0x40000000ULL) // 用户态 VMA 开始地址 1GB
#define KERNEL_VMA_END USER_VMA_START

// 内存引用计数 4kb对齐 从0开始
u8* page_map;

page_entry_t* page_table;  // 页目录

void* get_cr2()
{
    // 直接将 mov eax, cr2，返回值在 eax 中
    asm volatile("movl %cr2, %eax\n");
}

void* get_cr3(){
    asm volatile("movl %cr3,%eax\n");
}
void set_cr3(void* pde){
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
    entry->user = 1;    // 超级用户
    entry->index = index;
}

static void kernel_pte_init(); // 映射页

void memory_init()
{
    LOG("memory init\n");
    //内存状态的检测
    LOG("total mem size is %d MB\n",device_info->mem_upper >> 10);

    if (!(device_info->flags & (1 << 6)))panic("Cannot Scan Memory!");

    multiboot_memory_map_t *mmap;
    LOG("mmap_addr = 0x%x, mmap_length = 0x%x\n",(unsigned) device_info->mmap_addr, (unsigned) device_info->mmap_length);
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

    //grub载入内核也会载入到可用内存上 所以必须为内核空出一定的空间
    //grub是从低到高载入的 所以只需要让出低位的内存
    memory_base += KERNEL_MEM_SPACE;
    memory_size -= KERNEL_MEM_SPACE;

    LOG("mem base 0x%X size: 0x%X\n",memory_base,memory_size);

    total_page = IDX(memory_size);
    LOG("total page 0x%x\n",total_page);

    page_map = (void*)memory_base;  //开头的内存分配给内存表
    memory_base += ((total_page / PAGE_SIZE) + 1) * PAGE_SIZE;  //表要能记录total_page个内存
    memory_size -= ((total_page / PAGE_SIZE) + 1) * PAGE_SIZE;  //那么剩余的内存就减少了
    total_page -= (total_page / PAGE_SIZE) + 1;                 //总可用页数减少

    LOG("can use mem base 0x%X size: 0x%X\n",memory_base,memory_size);
    for(size_t index=0;index < total_page;index++)
        page_map[index] = 0; //设置为全0

    // 设置内核页表
    kernel_pte_init();
}

static void* get_page()
{
    // 从memory_base开始分配内存
    for(size_t index = IDX(memory_base) ; index < total_page ; index++)
    {
        if(page_map[index] == 0)
        {
            page_map[index] = 1;
            return PAGE(index);
        }
    }
    //panic("Out of Memory");
    return NULL;
}

static void put_page(void* addr)
{
    assert((u32)addr >= memory_base && (u32)addr < memory_base + memory_size); // 内存必须在可用区域
    size_t index = IDX((u32)addr - memory_base);
    assert(index < total_page); //不能比总页面数大
    assert(page_map[index] != 0);//引用次数不能为0
    page_map[index]--;  // 内存引用次数减少
}


struct page_error_code_t
{
    u8 present : 1;
    u8 write : 1;
    u8 user : 1;
    u8 reserved0 : 1;
    u8 fetch : 1;
    u8 protection : 1;
    u8 shadow : 1;
    u8 reserved1 : 8;
    u8 sgx : 1;
    u16 reserved2;
}__attribute__((packed)) page_error_code_t;


// 缺页中断
static void page_int(u32 vector,
                     u32 edi, u32 esi, u32 ebp, u32 esp,
                     u32 ebx, u32 edx, u32 ecx, u32 eax,
                     u32 gs, u32 fs, u32 es, u32 ds,
                     u32 vector0, u32 error, u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);

    void* vaddr = get_cr2();
    struct page_error_code_t *code = (struct page_error_code_t *)&error;

    LOG("fault virtual address 0x%p\n",vaddr);

    if (!code->present)
    {
        panic("no page!\n");
    }

    if (code->write)
    {
        panic("write read-only mem\n");
    }
fault:
    panic("unknown page fault!\n");
}

// 映射内核基本的内存
static void kernel_pte_init()
{
    //注册缺页中断
    interrupt_hardler_register(0x0e, page_int);

    page_table = get_page(); // 取一页内存用作页目录

    memset(page_table,0,PAGE_SIZE); // 全0可以使present为0 便于触发缺页中断
    LOG("page_table at 0x%x\n", page_table);

    u32 offset = 0;
    for(int i = 0;i < 4;i++) {
        page_entry_t* pte = get_page();

        memset(pte, 0, PAGE_SIZE);

        entry_init(&page_table[i], IDX((u32) pte));   // 页目录0->内核页表
        for (u32 index = 0; index < PAGE_SIZE / 4; index++)
        {
            entry_init(&pte[index], offset++);   // 映射物理内存在原来的位置
        }
    }

    //task_t* root_task = get_root_task();
    //for(u32 idx = 0;idx < )

    set_cr3(page_table); // cr3指向页目录
    enable_page();
}
// 只能映射4k对齐的页面
int add_mmap(void* vma,struct mm_struct* mm)
{

    return 0;
}

// 分配内核态内存
void* alloc_page(int page)
{
    void* ptr = NULL;
    task_t* root_task = get_root_task();
    void* root_task_bitmap = root_task->mm.mm_bitmap;

    //寻找连续的虚拟内存
    u32 index = IDX(KERNEL_VMA_START);
    for (; index < IDX(KERNEL_VMA_END); index++) {
        for (int pg = 0;; pg++) {
            if(bitmap_get(root_task_bitmap,index))
            {
                index += pg;
                break;
            }
            // 如果找到了 连续page个的页面则退出
            if (pg == page - 1)
            {
                goto scan_finish;
            }
        }
    }

scan_finish:
    ptr = index < IDX(KERNEL_VMA_END) ? PAGE(index) : NULL;

    return ptr;
}

int fork_mm_struct(struct mm_struct* child,struct mm_struct* father)
{
    child->pte = get_page();
    //复制一级页表
    memcpy(child->pte,father->pte,PAGE_SIZE);

    // 制作页表项指针，防止写错代码
    page_entry_t *child_pte = child->pte,
                    *father_pte = father->pte;

    for(u32 index=0;index < PTE_SIZE; index++)
    {
        // 当此页有效则开始复制
        if (child_pte[index].present)
        {
            // 为页表申请内存
            page_entry_t *sub_pte = get_page();
            child_pte[index].index = IDX(sub_pte);

            // 取父二级页表
            page_entry_t *fa_sub_pte = PAGE(father_pte[index].index);

            for (int sub_idx = 0;sub_idx < PTE_SIZE;sub_idx++)
            {
                // 取出当前页表项
                page_entry_t *fa_pet = fa_sub_pte + sub_idx;
                // 页表项不存在则不复制
                if(!fa_pet->present) continue;
                // 内存增加引用
                if (fa_pet->index >= IDX(USER_VMA_START))
                    page_map[fa_pet->index]++;  //超过255次引用会溢出（有空再改）

                 //内存只读 （内核态似乎能读写只读的内存 因此不会触发写时复制）
                fa_pet->write = 1;
            }
            // 复制页表
            memcpy(sub_pte,fa_sub_pte,PAGE_SIZE);
        }
    }
    return 0;
}

// void sys_mmap(void* addr,void* vaddr) {}