//
// Created by 谢子南 on 2024/4/13.
//

#include <nar/panic.h>
#include <memory.h>
#include <type.h>
#include "nar/globa.h"
#include "syscall.h"
#include "nar/mem.h"

#define CR0_PG (1 << 31)

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

static u32 get_cr0()
{
    u32 ret;
    asm volatile(
            "movl %%cr0,%%eax\n"
            :"=a"(ret)
            );
    return ret;
}

static void set_cr0(u32 cr0)
{
    asm volatile(
            "movl %%eax,%%cr0\n"
            ::"a"(cr0)
            );
}

static void disable_page()
{
    u32 cr0 = get_cr0();
    cr0 &= ~(CR0_PG);
    set_cr0(cr0);
}

static void enable_page()
{
    u32 cr0 = get_cr0();
    cr0 |= CR0_PG;
    set_cr0(cr0);
}

void cpuid(const u32 value,u32* const eax,u32* const ebx,u32* const ecx,u32* const edx)
{
    asm volatile(
            "cpuid\n"
            :"=a"(*eax),"=b"(*ebx),"=c"(*ecx),"=d"(*edx)
            :"a"(value)
            );
}

static int ap_id = 0;

__attribute__((unused)) void ap_initialize()
{
    u32 apic_id = *(u32 *)APIC_ID;
    apic_id = (apic_id >> 24) & 0xf; // P6 family and Pentium processors
    printk("I am cpu %d !\n",apic_id);
}

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
    //LOG("x2apic enable: %d\n",(apic_base >> 10) & 1);

    printk("bsp apic id is %x\n",*(u32 *)APIC_ID);

    // 复制AP启动代码到低1M内存
    extern void apup();
    memcpy((void*)0x0,apup,512);
    // 为16位AP复制gdt_48 idt_48
    __attribute__((unused)) extern pointer_t gdt_ptr;
    __attribute__((unused)) extern pointer_t idt_48;
    memcpy((void *) 0x1f0,&gdt_ptr,sizeof gdt_ptr);
    memcpy((void *) 0x200,&idt_48,sizeof idt_48);

    disable_page();
    mfence();   // 内存屏障

    u32* spurious = (u32*) SVR;
    LOG("SVR: %x\n",*spurious);
    // AP 启动序列
    LOG("wake up ap!\n");
    u32 *icr = (u32*)ICR_LOW;
    *icr = 0xc4500;
    //delay();  // 物理机要开，虚拟机就不用了
    *icr = 0xc4600;
    *icr = 0xc4600;

    enable_page();

}



