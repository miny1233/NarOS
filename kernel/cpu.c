//
// Created by 谢子南 on 2024/4/13.
//

#include <nar/panic.h>
#include <memory.h>
#include <type.h>

#define ICR_LOW 0x0FEE00300UL
#define SVR 0x0FEE000F0UL
#define APIC_ID 0x0FEE00020UL
#define LVT3 0x0FEE00370UL
#define APIC_ENABLED 0x0100UL

static u32 rdmsr(u32 addr)
{
    u32 ret;
    asm volatile(
            "rdmsr\n"
            :"=a"(ret):"c"(addr)
            );
    return ret;
}

static void wrmsr(u32 addr,u32 low)
{
    asm volatile(
            "xor %%edx,%%edx\n"
            "wrmsr\n"
            ::"c"(addr),"a"(low)
            );
}

static void change_page()   // 启用分页
{
    asm volatile(
            "movl %cr0, %eax\n"
            "xorl $0x80000000, %eax\n"
            "movl %eax, %cr0\n");
}


static void delay()
{
    int wait = 1e9;
    while(wait--);
}

void cpuid(u32 value,u32* eax,u32* ebx,u32* ecx,u32* edx)
{
    asm volatile(
            "cpuid\n"
            :"=a"(*eax),"=b"(*ebx),"=c"(*ecx),"=d"(*edx)
            :"a"(value)
            );
}

void ap_initialize()
{
    while(1)
        printk("I am AP!\n");
}


extern void apup();

void cpu_init()
{
    // 检查x2APIC支持
    u32 eax=1,ebx,ecx,edx;
    cpuid(0x1,&eax,&ebx,&ecx,&edx);
    if((ecx >> 21) & 1)
    {
        LOG("enable x2APIC\n");
        u32 apic_base = rdmsr(0x1b);
        apic_base |= (1<<10);
        wrmsr(0x1b,apic_base);
    }

    LOG("not supported x2APIC\n");

    u32 apic_base;
    apic_base = rdmsr(0x1b);
    LOG("apic base: %p\n",apic_base & ~(0xfff));
    LOG("bsp: %d\n",(apic_base >> 8) & 1);
    LOG("apic global: %d\n",(apic_base >> 11) & 1);
    LOG("x2apic enable: %d\n",(apic_base >> 10) & 1);

    // 复制AP启动代码到低1M内存
    memcpy((void*)0x0,apup,512);

    change_page();

    u32* spurious = (u32*) SVR;
    LOG("SVR: %x\n",*spurious);
    // AP 启动序列
    LOG("wake up ap!\n");
    u32 *icr = (u32*)ICR_LOW;
    LOG("send init\n");
    *icr = 0xc4500;
    //delay();  // 物理机要开，虚拟机就不用了
    LOG("send startup\n");
    *icr = 0xc4600;
    //delay();
    LOG("send startup\n");
    *icr = 0xc4600;

    change_page();

}



