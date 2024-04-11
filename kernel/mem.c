#include <nar/mem.h>
#include <type.h>
#include <nar/panic.h>
#include <nar/interrupt.h>
#include <memory.h>
#include <nar/multiboot.h>
#include <math.h>
#include <nar/task.h>
#include <bitmap.h>

u32 memory_base = 0;  //由于GRUB把内核载入到了1M处，为了保证安全至少从2M开始有效
u32 memory_size = 0;
u32 total_page;

#define DIDX(addr) (((u32)(addr) >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((u32)(addr) >> 12) & 0x3ff) // 获取 addr 的页表索引
#define IDX(addr) ((u32)(addr) >> 12) // 取页索引
#define PAGE(idx) ((void*)((u32)(idx) << 12))  // 取页启始

#define PDE_MASK 0xFFC00000
#define BITMAP_MASK 0xFFB00000 //专为内存位图设计的内存位置

#define PTE_SIZE (PAGE_SIZE/sizeof(page_entry_t))

#define KERNEL_MEM_SPACE (0xE00000ULL) // 16MB 内核专用内存 (内核代码段 与 数据段) 内存分配时绕过这块物理内存

#define KERNEL_HEAP_VMA_START ((void*)0x2000000ULL)
#define USER_VMA_START  ((void*)0x40000000ULL) // 用户态 VMA 开始地址 1GB
#define USER_VMA_END (BITMAP_MASK << 10)
#define KERNEL_HEAP_VMA_END USER_VMA_START

#define KERNEL_DPL 0
#define USER_DPL 3

// 内存引用计数 4kb对齐 从0开始
u8* page_map;

page_entry_t* page_table;  // 页目录

static void* ksbrk = KERNEL_HEAP_VMA_START;

void flush_tlb(void* vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr)
            : "memory");
}

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

u32 get_cr4()
{
    asm volatile("movl %cr4,%eax\n");
}

void set_cr4(u32 value)
{
    asm volatile("movl %%eax,%%cr4\n"::"a"(value));
}


static void enable_page()   // 启用分页
{
    asm volatile(
            "movl %cr0, %eax\n"
            "orl $0x80000000, %eax\n"
            "movl %eax, %cr0\n");
}

// 获取虚拟地址 vaddr 对应的页表
static page_entry_t *get_pte(void* vaddr)
{
    u32 idx = DIDX(vaddr);

    page_entry_t *table = (page_entry_t *)(PDE_MASK | (idx << 12));

    return table;
}

page_entry_t *get_entry(void* vaddr)
{
    page_entry_t *pte = get_pte(vaddr);
    return &pte[TIDX(vaddr)];
}

// 获取当前空间下虚拟地址 vaddr 对应的物理地址
void* get_paddr(void* vaddr)
{
    page_entry_t *pde = running->mm->pde;
    page_entry_t *entry = &pde[DIDX(vaddr)];
    if (!entry->present)
        return 0;

    entry = get_entry(vaddr);
    if (!entry->present)
        return 0;

    return (void*)((u32)PAGE(entry->index) | ((u32)vaddr & 0xfff));
}

// 初始化页表项
static void entry_init(page_entry_t *entry, u32 index ,char dpl)
{
    *(u32 *)entry = 0;
    entry->present = 1;
    entry->write = 1;
    if (KERNEL_DPL == dpl)
    {
        entry->user = 0;
        entry->global = 1;
    } else{
        entry->user = 1;
    }
    entry->global = 1;
    entry->index = index;
}

static void kernel_pte_init(); // 映射页

void memory_init()
{
    LOG("memory init\n");
    //内存状态的检测
    LOG("total mem size is %d MB\n",device_info->mem_upper >> 10);

    if (!(device_info->flags & (1 << 6)))
        panic("Cannot Scan Memory!");

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

void* get_page()
{
    // 从memory_base开始分配内存
    for(size_t index = IDX(memory_base) ; index < total_page ; index++)
    {
        if(page_map[index] == 0)
        {
            page_map[index] = 1;
            printk("get page 0x%x\n",PAGE(index));
            return PAGE(index);
        }
    }
    //panic("Out of Memory");
    return NULL;
}

void put_page(void* addr)
{
    assert((u32)addr >= memory_base && (u32)addr < memory_base + memory_size); // 内存必须在可用区域
    size_t index = IDX((u32)addr - memory_base);
    assert(index < total_page); //不能比总页面数大
    assert(page_map[index] != 0);//引用次数不能为0
    page_map[index]--;  // 内存引用次数减少
}

// 标记使用的内存
static void* recorde_used_page(struct mm_struct* mm,void* paddr)
{
    //记录使用的内存
    //bitmap_set(mm->pm_bitmap,IDX(paddr),1);
    return paddr;
}

//内核内存描述符初始化
int init_mm_struct(struct mm_struct* mm)
{
    mm->pde = get_cr3();
    page_entry_t* self = mm->pde + DIDX(PDE_MASK);
    entry_init(self, IDX(mm->pde) ,KERNEL_DPL);
    //堆内存初始化
    mm->brk = (void*) USER_VMA_START;
    mm->sbrk = mm->brk;

    return 0;
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

// 透穿模式
// 用于跨地址空间的写入
static struct mm_struct* proxy_mm = NULL;

// 复制数据到mm所描述的虚拟内存空间下
// @sou : 必须是在内核空间的数据
// @des : 必须是有效的虚拟地址
// 函数堆栈必须在内核区域内
void* copy_to_mm_space(struct mm_struct *mm,void* des,void* sou,size_t len)
{
    // 强制缺页中断时使用指定mm
    proxy_mm = mm;
    // 保存并切换页目录
    void* old_pde = get_cr3();
    set_cr3(mm->pde);

    memcpy(des,sou,len);

    //恢复页目录
    set_cr3(old_pde);
    proxy_mm = NULL;

    return des;
}

// 缺页中断
static void page_int(int vector,
                     u32 edi, u32 esi, u32 ebp, u32 esp,
                     u32 ebx, u32 edx, u32 ecx, u32 eax,
                     u32 gs, u32 fs, u32 es, u32 ds,u32 error,
                     u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);

    void* vaddr = get_cr2();
    struct page_error_code_t *code = (struct page_error_code_t *)&error;

    LOG("fault virtual address 0x%p\n",vaddr);

    //用户态态故障 且 访问内存位置不合法
    if (running->dpl == 3 && vaddr < USER_VMA_START)
        goto segment_error;

    //取出内存描述符
    struct mm_struct* mm = proxy_mm ? proxy_mm : running->mm;

    //检查是否在堆内存 内核进程跳过检查
    if(running->dpl == 0 || (vaddr >= mm->brk && vaddr < mm->sbrk))
        goto check_passed;
    //检查 virtual memory area （暂未启用）
    goto segment_error;

check_passed:

    if (!code->present)
    {
        //取出虚拟地址在页目录的表项
        page_entry_t *page_table_fir = mm->pde + DIDX(vaddr);
        //缺少页表
        if(page_table_fir->present == 0)
        {
            page_entry_t* page_table_sec = get_page();
            if(page_table_sec == NULL)
                goto fault;

            entry_init(page_table_fir,IDX(page_table_sec),vaddr < USER_VMA_START ? KERNEL_DPL : USER_DPL);

            recorde_used_page(mm,page_table_sec);
        }
        // 初始化页表
        page_entry_t* page_table_sec = get_pte(vaddr);
        flush_tlb(page_table_sec);

        // 大坑，之前每次都擦除一遍页表
        if(page_table_fir->present == 0)
            memset(page_table_sec,0,PAGE_SIZE);

        page_table_sec += TIDX(vaddr);

        //分配物理内存
        void* pm = get_page();
        if(pm == NULL)
            goto fault;

        entry_init(page_table_sec, IDX(pm), vaddr < USER_VMA_START ? KERNEL_DPL : USER_DPL);

        recorde_used_page(mm,pm);
        goto ok;
    }
    else if (code->write)
    {
        page_entry_t* pte = get_pte(vaddr);

        LOG("present %d\n",pte->present);

        goto ok;
        //panic("write read-only mem\n");
    }

    goto segment_error;

ok:
    flush_tlb(vaddr);
    return;
segment_error:
    printk("segment fault!\n");
fault:
    if(running->dpl == 0)
        panic("page fault!\n");
    else
        task_exit();
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

        entry_init(&page_table[i], IDX((u32) pte),KERNEL_DPL);   // 页目录0->内核页表
        for (u32 index = 0; index < PAGE_SIZE / 4; index++)
        {
            entry_init(&pte[index], offset++,KERNEL_DPL);   // 映射物理内存在原来的位置
        }
    }

    //task_t* root_task = get_root_task();
    //for(u32 idx = 0;idx < )

    set_cr3(page_table); // cr3指向页目录
    // Enable PGE
    u32 cr4 = get_cr4();
    cr4 |= 1 << 7;
    set_cr4(cr4);

    enable_page();
}

//推动堆指针
void* sbrk(struct mm_struct* mm,int increase)
{
    void* ptr = NULL;

    if(mm->sbrk + increase < (void*) USER_VMA_END)
    {
        ptr = mm->sbrk;
        mm->sbrk += increase;
    }

    return ptr;
}

void* kbrk(int increase)
{
    void* ptr = NULL;

    if(ksbrk + increase < (void*) USER_VMA_START)
    {
        ptr = ksbrk;
        ksbrk += increase;
    }

    return ptr;
}

// 按页分配内存
void* kalloc_page(int page)
{
    return kbrk(page * PAGE_SIZE);
}

void init_user_mm_struct(struct mm_struct* mm)
{
    // 初始化页目录
    void* pde_vaddr = kbrk(PAGE_SIZE);
    // 复制内核页表
    memcpy(pde_vaddr,get_root_task()->mm->pde,PAGE_SIZE);

    //取得页表物理地址
    mm->pde = get_paddr(pde_vaddr);

    //为页表创建一个自身连接
    page_entry_t* self = mm->pde + DIDX(PDE_MASK);
    entry_init(self, IDX(mm->pde) ,KERNEL_DPL);

    // 初始化堆
    mm->sbrk = USER_VMA_START;
    mm->brk = USER_VMA_START;
}

//为fork准备的内存描述复制
int fork_mm_struct(struct mm_struct* child,struct mm_struct* father)
{
    child->pde = get_page();
    //复制一级页目录
    memcpy(child->pde,father->pde,PAGE_SIZE);

    // 制作页表项指针，防止写错代码
    page_entry_t *child_pte = child->pde,
                    *father_pte = father->pde;

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


