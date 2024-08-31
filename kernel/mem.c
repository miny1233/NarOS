#include <nar/mem.h>
#include <type.h>
#include <nar/panic.h>
#include <nar/interrupt.h>
#include <memory.h>
#include <nar/multiboot.h>
#include <math.h>
#include <nar/task.h>
#include <bitmap.h>
#include <nar/heap.h>

u32 memory_base = 0;  //由于GRUB把内核载入到了1M处，为了保证安全至少从2M开始有效
u32 memory_size = 0;
u32 total_page;

#define DIDX(addr) (((u32)(addr) >> 22) & 0x3ff) // 获取 addr 的页目录索引
#define TIDX(addr) (((u32)(addr) >> 12) & 0x3ff) // 获取 addr 的页表索引
#define IDX(addr) ((u32)(addr) >> 12) // 取页索引
#define PAGE(idx) ((void*)((u32)(idx) << 12))  // 取页启始

#define APIC_MASK 0xFEE00000UL
#define PDE_MASK 0xFFC00000UL
#define BITMAP_MASK 0xFFB00000UL //专为内存位图设计的内存位置

#define PTE_SIZE (PAGE_SIZE/sizeof(page_entry_t))

#define KERNEL_MEM_SPACE (0x1000000UL) // 16MB 内核专用内存 (内核代码段 与 数据段) 内存分配时绕过这块物理内存

#define KERNEL_HEAP_VMA_START ((void*)0x2000000UL)
#define USER_VMA_START  ((void*)0x40000000UL) // 用户态 VMA 开始地址 1GB
#define USER_VMA_END (BITMAP_MASK << 10)
#define KERNEL_HEAP_VMA_END USER_VMA_START

#define KERNEL_DPL 0
#define USER_DPL 3

// 内存引用计数 4kb对齐 从0开始
u8* page_map;

__attribute__ ((aligned (4096)))
page_entry_t page_dic[1024];  // 内核页目录
__attribute__ ((aligned (4096)))
page_entry_t page_tables[256][1024]; // 内核页表 (共1GB)

static void* ksbrk = KERNEL_HEAP_VMA_START;

void flush_tlb(void* vaddr)
{
    asm volatile("invlpg (%0)" ::"r"(vaddr)
            : "memory");
}

void* get_cr2()
{
    void* ret;
    // 直接将 mov eax, cr2，返回值在 eax 中
    asm volatile(
            "movl %%cr2, %%eax\n"
            :"=a"(ret));
    return ret;
}

void* get_cr3(){
    void* ret;
    asm volatile(
            "movl %%cr3,%%eax\n"
            :"=a"(ret));
    return ret;
}

void set_cr3(void* pde){
    asm volatile("movl %%eax,%%cr3\n"::"a"(pde));
}

u32 get_cr4()
{
    u32 ret;
    asm volatile("movl %%cr4,%%eax\n":"=a"(ret));
    return ret;
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
    } else{
        entry->user = 1;
    }
    entry->index = index;
}

void* get_page()
{
    // 从memory_base开始分配内存
    for(size_t index = IDX(memory_base) ; index < total_page ; index++)
    {
        if(page_map[index] == 0)
        {
            page_map[index] = 1;
            //LOG(" get page 0x%x\n",PAGE(index));
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
                     u32 gs,  u32 fs, u32 es, u32 ds,u32 error,
                     u32 eip, u32 cs, u32 eflags)
{
    assert(vector == 0xe);

    void* vaddr = get_cr2();
    struct page_error_code_t *code = (struct page_error_code_t *)&error;

    LOG("fault virtual address 0x%p\n",vaddr);

    int dpl = cs == 8 ? 0 : 3;
    //用户态态故障 且 访问内存位置不合法
    if (dpl == 3 && vaddr < USER_VMA_START)
        goto segment_error;

    /*
     *  取出内存描述符
     *  proxy_mm是为copy_to_mm_space设计的，当proxy_mm被设置后，
     *  应该使用指定的mm所指向的内存空间
     */
    struct mm_struct* mm = proxy_mm ? proxy_mm : running->mm;

    //检查是否在堆内存 内核进程跳过检查
    if(dpl == 0 || (vaddr >= mm->brk && vaddr < mm->sbrk))
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
            // 内核区域不可能出现缺少页表
            assert(vaddr >= USER_VMA_START);

            page_entry_t* page_table_sec = get_page();
            if(page_table_sec == NULL)
                goto fault;

            entry_init(page_table_fir,IDX(page_table_sec),vaddr < USER_VMA_START ? KERNEL_DPL : USER_DPL);
            memset(page_table_sec,0,PAGE_SIZE);

            recorde_used_page(mm,page_table_sec);
        }
        // 初始化页表
        page_entry_t* page_table_sec = get_pte(vaddr);
        flush_tlb(page_table_sec);
        // 移动到对应到页表项
        page_table_sec += TIDX(vaddr);

        // 分配物理内存
        void* pm = get_page();
        if(pm == NULL)
            goto fault;
        // 映射物理内存
        entry_init(page_table_sec, IDX(pm), vaddr < USER_VMA_START ? KERNEL_DPL : USER_DPL);

        // 只记录用户态的内存占用 内核堆由其他程序管理
        if (vaddr >= USER_VMA_START)
            recorde_used_page(mm,pm);

        goto ok;
    }
    else if (code->write)
    {
        panic("Not support Copy on Write!\n");
    }

    goto segment_error;

ok:
    flush_tlb(vaddr);
    return;
segment_error:
    printk("segment fault!\n");
fault:
    if(dpl == 0)
        panic("page fault!\n");
    else
        task_exit();
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

void* kbrk(int size)
{
    // LOG("kbrk called size is: %d\n",size);
    void* ret = ksbrk;
    ksbrk += size;
    return ret;
}

void init_user_mm_struct(struct mm_struct* mm)
{
    // 获得页目录内存
    size_t pde_mem = (size_t)kalloc(2 * PAGE_SIZE);
    // 4KB对齐
    pde_mem += PAGE_SIZE;
    pde_mem &= ~(0xfff);

    // 设置页目录内存地址
    void* pde_vaddr = (void*)pde_mem;
    // 复制内核页目录
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

void free_user_mm_struct(struct mm_struct* mm)
{

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

// 映射内核基本的内存
static void kernel_pte_init()
{
    //注册缺页中断
    interrupt_hardler_register(0x0e, page_int);

    memset(page_dic,0,sizeof (page_dic));  // 初始化页目录
    memset (page_tables,0,sizeof (page_tables)); // 初始化内核页表
    // 将所有的内核页表都映射上 保证内核页面一定是共享的
    for (size_t idx = 0;idx < 256;idx++)
    {
        entry_init(page_dic + idx, IDX(page_tables[idx]),USER_DPL);
    }
    // 将内核的现在占用的空间直接映射 32MB
    for (size_t idx = 0;idx < 8;idx++)
    {
        // 一个页表映射4MB空间
        for (size_t mapping_count = 0;mapping_count < 1024;mapping_count++)
        {
            static size_t offset = 0;
            // 每次映射4KB
            entry_init(&page_tables[idx][mapping_count],offset++,USER_DPL);
        }
    }

    //  映射APCI内存 （完全没用 无论如何读写都是0 需要一个IA32大佬）

    page_entry_t *apic_pte = get_page();
    for (u32 index = 0; index < 1024; index++)
    {
        u32 apci_reg_addr = IDX(APIC_MASK) + index;
        entry_init(apic_pte + index, apci_reg_addr,KERNEL_DPL);
        apic_pte[index].pwt = 1;
        apic_pte[index].pcd = 1;
    }
    entry_init((page_entry_t *) (page_dic + DIDX(APIC_MASK)), IDX(apic_pte), KERNEL_DPL);
    ((page_entry_t *)page_dic + DIDX(APIC_MASK))->pwt = 1;
    ((page_entry_t *)page_dic + DIDX(APIC_MASK))->pcd = 1;


    set_cr3(&page_dic); // cr3指向页目录
    enable_page();
}


