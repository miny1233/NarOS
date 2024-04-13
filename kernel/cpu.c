//
// Created by 谢子南 on 2024/4/13.
//

#include <nar/panic.h>
#include <type.h>

#define ICR_LOW 0x0FEE00300
#define SVR 0x0FEE000F0
#define APIC_ID 0x0FEE00020
#define LVT3 0x0FEE00370
#define APIC_ENABLED 0x0100

static u32 rdmsr(u32 addr)
{
    u32 ret;
    asm volatile(
            "rdmsr"
            :"=a"(ret):"c"(addr)
            );
    return ret;
}

void cpu_init()
{
    u32 apic_base = rdmsr(0x1b);
    LOG("apic base: %p\n",apic_base & ~(0xfff));
    LOG("bsp: %d\n",(apic_base >> 8)&1);
    LOG("apic global: %d\n",(apic_base >> 11) & 1);

    u32* spurious = (u32*) 0xfee000f0;
    *spurious = 1 << 8 | 0x20;
    LOG("spurious: %x\n",*spurious);

    *(u32 *) 0xfee00320 = (1 << 17) | (1 << 12) | 0x20;
    LOG("timer: %x\n",*(u32 *) 0xfee00320);

    u32 *current = (u32 *) 0xfee00390;
    u32 *initial = (u32 *) 0xfee00380;
    LOG("current: %u initial: %u\n", current, initial);
}



