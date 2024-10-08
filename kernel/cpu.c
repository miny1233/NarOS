//
// Created by 谢子南 on 2024/4/13.
//

#include <nar/panic.h>
#include <memory.h>
#include <type.h>
#include "nar/globa.h"
#include "syscall.h"
#include "nar/mem.h"
#include "device/io.h"
#include <nar/irq_vectors.h>

#define CR0_PG (1 << 31)

#define ICR_LOW 0x0FEE00300UL
#define SVR 0x0FEE000F0UL
#define APIC_BASE 0x0FEE00020UL
#define LVT3 0x0FEE00370UL
#define APIC_ENABLED 0x0100UL

#define IO_APIC_SLOT_SIZE		1024

#define APIC_DELIVERY_MODE_FIXED	0
#define APIC_DELIVERY_MODE_LOWESTPRIO	1
#define APIC_DELIVERY_MODE_SMI		2
#define APIC_DELIVERY_MODE_NMI		4
#define APIC_DELIVERY_MODE_INIT		5
#define APIC_DELIVERY_MODE_EXTINT	7

#define	APIC_ID		0x20

#define	APIC_LVR	0x30
#define		APIC_LVR_MASK		0xFF00FF
#define		APIC_LVR_DIRECTED_EOI	(1 << 24)
#define		GET_APIC_VERSION(x)	((x) & 0xFFu)
#define		GET_APIC_MAXLVT(x)	(((x) >> 16) & 0xFFu)
#define	APIC_INTEGRATED(x)	((x) & 0xF0u)
#define		APIC_XAPIC(x)		((x) >= 0x14)
#define		APIC_EXT_SPACE(x)	((x) & 0x80000000)
#define	APIC_TASKPRI	0x80
#define		APIC_TPRI_MASK		0xFFu
#define	APIC_ARBPRI	0x90
#define		APIC_ARBPRI_MASK	0xFFu
#define	APIC_PROCPRI	0xA0
#define	APIC_EOI	0xB0
#define		APIC_EOI_ACK		0x0 /* Docs say 0 for future compat. */
#define	APIC_RRR	0xC0
#define	APIC_LDR	0xD0
#define		APIC_LDR_MASK		(0xFFu << 24)
#define		GET_APIC_LOGICAL_ID(x)	(((x) >> 24) & 0xFFu)
#define		SET_APIC_LOGICAL_ID(x)	(((x) << 24))
#define		APIC_ALL_CPUS		0xFFu
#define	APIC_DFR	0xE0
#define		APIC_DFR_CLUSTER		0x0FFFFFFFul
#define		APIC_DFR_FLAT			0xFFFFFFFFul
#define	APIC_SPIV	0xF0
#define		APIC_SPIV_DIRECTED_EOI		(1 << 12)
#define		APIC_SPIV_FOCUS_DISABLED	(1 << 9)
#define		APIC_SPIV_APIC_ENABLED		(1 << 8)
#define	APIC_ISR	0x100
#define	APIC_ISR_NR     0x8     /* Number of 32 bit ISR registers. */
#define	APIC_TMR	0x180
#define	APIC_IRR	0x200
#define	APIC_ESR	0x280
#define		APIC_ESR_SEND_CS	0x00001
#define		APIC_ESR_RECV_CS	0x00002
#define		APIC_ESR_SEND_ACC	0x00004
#define		APIC_ESR_RECV_ACC	0x00008
#define		APIC_ESR_SENDILL	0x00020
#define		APIC_ESR_RECVILL	0x00040
#define		APIC_ESR_ILLREGA	0x00080
#define 	APIC_LVTCMCI	0x2f0
#define	APIC_ICR	0x300
#define		APIC_DEST_SELF		0x40000
#define		APIC_DEST_ALLINC	0x80000
#define		APIC_DEST_ALLBUT	0xC0000
#define		APIC_ICR_RR_MASK	0x30000
#define		APIC_ICR_RR_INVALID	0x00000
#define		APIC_ICR_RR_INPROG	0x10000
#define		APIC_ICR_RR_VALID	0x20000
#define		APIC_INT_LEVELTRIG	0x08000
#define		APIC_INT_ASSERT		0x04000
#define		APIC_ICR_BUSY		0x01000
#define		APIC_DEST_LOGICAL	0x00800
#define		APIC_DEST_PHYSICAL	0x00000
#define		APIC_DM_FIXED		0x00000
#define		APIC_DM_FIXED_MASK	0x00700
#define		APIC_DM_LOWEST		0x00100
#define		APIC_DM_SMI		0x00200
#define		APIC_DM_REMRD		0x00300
#define		APIC_DM_NMI		0x00400
#define		APIC_DM_INIT		0x00500
#define		APIC_DM_STARTUP		0x00600
#define		APIC_DM_EXTINT		0x00700
#define		APIC_VECTOR_MASK	0x000FF
#define	APIC_ICR2	0x310
#define		GET_XAPIC_DEST_FIELD(x)	(((x) >> 24) & 0xFF)
#define		SET_XAPIC_DEST_FIELD(x)	((x) << 24)
#define	APIC_LVTT	0x320
#define	APIC_LVTTHMR	0x330
#define	APIC_LVTPC	0x340
#define	APIC_LVT0	0x350
#define		APIC_LVT_TIMER_ONESHOT		(0 << 17)
#define		APIC_LVT_TIMER_PERIODIC		(1 << 17)
#define		APIC_LVT_TIMER_TSCDEADLINE	(2 << 17)
#define		APIC_LVT_MASKED			(1 << 16)
#define		APIC_LVT_LEVEL_TRIGGER		(1 << 15)
#define		APIC_LVT_REMOTE_IRR		(1 << 14)
#define		APIC_INPUT_POLARITY		(1 << 13)
#define		APIC_SEND_PENDING		(1 << 12)
#define		APIC_MODE_MASK			0x700
#define		GET_APIC_DELIVERY_MODE(x)	(((x) >> 8) & 0x7)
#define		SET_APIC_DELIVERY_MODE(x, y)	(((x) & ~0x700) | ((y) << 8))
#define			APIC_MODE_FIXED		0x0
#define			APIC_MODE_NMI		0x4
#define			APIC_MODE_EXTINT	0x7
#define	APIC_LVT1	0x360
#define	APIC_LVTERR	0x370
#define	APIC_TMICT	0x380
#define	APIC_TMCCT	0x390
#define	APIC_TDCR	0x3E0
#define APIC_SELF_IPI	0x3F0
#define		APIC_TDR_DIV_TMBASE	(1 << 2)
#define		APIC_TDR_DIV_1		0xB
#define		APIC_TDR_DIV_2		0x0
#define		APIC_TDR_DIV_4		0x1
#define		APIC_TDR_DIV_8		0x2
#define		APIC_TDR_DIV_16		0x3
#define		APIC_TDR_DIV_32		0x8
#define		APIC_TDR_DIV_64		0x9
#define		APIC_TDR_DIV_128	0xA
#define	APIC_EFEAT	0x400
#define	APIC_ECTRL	0x410
#define APIC_EILVTn(n)	(0x500 + 0x10 * n)
#define		APIC_EILVT_NR_AMD_K8	1	/* # of extended interrupts */
#define		APIC_EILVT_NR_AMD_10H	4
#define		APIC_EILVT_NR_MAX	APIC_EILVT_NR_AMD_10H
#define		APIC_EILVT_LVTOFF(x)	(((x) >> 4) & 0xF)
#define		APIC_EILVT_MSG_FIX	0x0
#define		APIC_EILVT_MSG_SMI	0x2
#define		APIC_EILVT_MSG_NMI	0x4
#define		APIC_EILVT_MSG_EXT	0x7
#define		APIC_EILVT_MASKED	(1 << 16)

#define APIC_BASE_MSR		0x800
#define APIC_X2APIC_ID_MSR	0x802
#define XAPIC_ENABLE		BIT(11)
#define X2APIC_ENABLE		BIT(10)

#ifdef CONFIG_X86_32
# define MAX_IO_APICS 64
# define MAX_LOCAL_APIC 256
#else
# define MAX_IO_APICS 128
# define MAX_LOCAL_APIC 32768
#endif

#define mb() __asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory")

#define PC_CONF_INDEX		0x22
#define PC_CONF_DATA		0x23

#define PC_CONF_MPS_IMCR	0x70

static inline u8 pc_conf_get(u8 reg)
{
    outb(reg, PC_CONF_INDEX);
    return inb(PC_CONF_DATA);
}

static inline void pc_conf_set(u8 reg, u8 data)
{
    outb(reg, PC_CONF_INDEX);
    outb(data, PC_CONF_DATA);
}

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

u32 apic_read(u32 reg)
{
    u32 value;
    mb();
    value = *(volatile u32 *)APIC_BASE + reg;
    mb();
    return value;
}

void apic_write(u32 reg,u32 value)
{
    mb();
    *(volatile u32 *)(APIC_BASE + reg) = value;
    mb();
}

static inline void imcr_pic_to_apic(void)
{
    /* NMI and 8259 INTR go through APIC */
    pc_conf_set(PC_CONF_MPS_IMCR, 0x01);
}

/*
 * Get the LAPIC version
 */
static inline int lapic_get_version(void)
{
    return GET_APIC_VERSION(apic_read(APIC_LVR));
}

/*
 * Check, if the APIC is integrated or a separate chip
 */
static inline int lapic_is_integrated(void)
{
    return APIC_INTEGRATED(lapic_get_version());
}

static int ap_id = 0;
__attribute__((unused)) void ap_initialize()
{
    u32 apic_id = *(u32 *)APIC_ID;
    apic_id = (apic_id >> 24) & 0xf; // P6 family and Pentium processors
    printk("I am cpu %d !\n",apic_id);
}

void ap_init()
{
    // 复制AP启动代码到低1M内存
    extern void apup();
    memcpy((void*)0x0,apup,512);
    // 为16位AP复制gdt_48 idt_48
    __attribute__((unused)) extern pointer_t gdt_ptr;
    __attribute__((unused)) extern pointer_t idt_48;
    memcpy((void *) 0x1f0,&gdt_ptr,sizeof gdt_ptr);
    memcpy((void *) 0x200,&idt_48,sizeof idt_48);

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
}

static void init_bsp_apic(void)
{
    LOG("Enable APIC\n");
    unsigned int value;
    /*
	 * Enable APIC.
	 */
    value = apic_read(APIC_SPIV);
    value &= ~APIC_VECTOR_MASK;
    value |= APIC_SPIV_APIC_ENABLED;
    //if ((boot_cpu_data.x86_vendor == X86_VENDOR_INTEL) && (boot_cpu_data.x86 == 15))
    if (0)
        value &= ~APIC_SPIV_FOCUS_DISABLED;
    else
        value |= APIC_SPIV_FOCUS_DISABLED;
    value |= SPURIOUS_APIC_VECTOR;
    apic_write(APIC_SPIV, value);

    /*
	 * Set up the virtual wire mode.
	 */
    apic_write(APIC_LVT0, APIC_DM_EXTINT);
    value = APIC_DM_NMI;
    if (!lapic_is_integrated())		/* 82489DX */
        value |= APIC_LVT_LEVEL_TRIGGER;
    // if (apic_extnmi == APIC_EXTNMI_NONE)
        value |= APIC_LVT_MASKED;
    apic_write(APIC_LVT1, value);
}

/**
 * setup_local_APIC - setup the local APIC
 *
 * Used to setup local APIC while initializing BSP or bringing up APs.
 * Always called with preemption disabled.
 */
static void setup_local_APIC(void)
{
    unsigned int value;

    /*
     * If this comes from kexec/kcrash the APIC might be enabled in
     * SPIV. Soft disable it before doing further initialization.
     */
    value = apic_read(APIC_SPIV);
    value &= ~APIC_SPIV_APIC_ENABLED;
    apic_write(APIC_SPIV, value);


    /* Pound the ESR really hard over the head with a big hammer - mbligh */
	if (lapic_is_integrated()) {
		apic_write(APIC_ESR, 0);
		apic_write(APIC_ESR, 0);
		apic_write(APIC_ESR, 0);
		apic_write(APIC_ESR, 0);
	}

    /*
     * Intel recommends to set DFR, LDR and TPR before enabling
     * an APIC.  See e.g. "AP-388 82489DX User's Manual" (Intel
     * document number 292116).
     *
     *
     */

    /*
     * Set Task Priority to 'accept all except vectors 0-31'.  An APIC
     * vector in the 16-31 range could be delivered if TPR == 0, but we
     * would think it's an exception and terrible things will happen.  We
     * never change this later on.
     */
    value = apic_read(APIC_TASKPRI);
    value &= ~APIC_TPRI_MASK;
    value |= 0x10;
    apic_write(APIC_TASKPRI, value);


    /*
     * Now that we are all set up, enable the APIC
     */
    value = apic_read(APIC_SPIV);
    value &= ~APIC_VECTOR_MASK;
    /*
     * Enable APIC
     */
    value |= APIC_SPIV_APIC_ENABLED;

#ifdef CONFIG_X86_32
    /*
	 * Some unknown Intel IO/APIC (or APIC) errata is biting us with
	 * certain networking cards. If high frequency interrupts are
	 * happening on a particular IOAPIC pin, plus the IOAPIC routing
	 * entry is masked/unmasked at a high rate as well then sooner or
	 * later IOAPIC line gets 'stuck', no more interrupts are received
	 * from the device. If focus CPU is disabled then the hang goes
	 * away, oh well :-(
	 *
	 * [ This bug can be reproduced easily with a level-triggered
	 *   PCI Ne2000 networking cards and PII/PIII processors, dual
	 *   BX chipset. ]
	 */
	/*
	 * Actually disabling the focus CPU check just makes the hang less
	 * frequent as it makes the interrupt distribution model be more
	 * like LRU than MRU (the short-term load is more even across CPUs).
	 */

	/*
	 * - enable focus processor (bit==0)
	 * - 64bit mode always use processor focus
	 *   so no need to set it
	 */
	value &= ~APIC_SPIV_FOCUS_DISABLED;
#endif

    /*
     * Set spurious IRQ vector
     */
    value |= SPURIOUS_APIC_VECTOR;
    apic_write(APIC_SPIV, value);

    /*
     * Set up LVT0, LVT1:
     *
     * set up through-local-APIC on the boot CPU's LINT0. This is not
     * strictly necessary in pure symmetric-IO mode, but sometimes
     * we delegate interrupts to the 8259A.
     */
    /*
     * TODO: set up through-local-APIC from through-I/O-APIC? --macro
     */
    value = apic_read(APIC_LVT0) & APIC_LVT_MASKED;
    value = APIC_DM_EXTINT;

    apic_write(APIC_LVT0, value);

    /*
     * Only the BSP sees the LINT1 NMI signal by default. This can be
     * modified by apic_extnmi= boot option.
     */
    value = APIC_DM_NMI;


    /* Is 82489DX ? */
    if (!lapic_is_integrated())
        value |= APIC_LVT_LEVEL_TRIGGER;
    apic_write(APIC_LVT1, value);

#ifdef CONFIG_X86_MCE_INTEL
    /* Recheck CMCI information after local APIC is up on CPU #0 */
	if (!cpu)
		cmci_recheck();
#endif
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
    }else {
        LOG("not supported x2APIC\n");

    }

    imcr_pic_to_apic();
    setup_local_APIC();
    init_bsp_apic();

    u32 apic_base;
    apic_base = rdmsr(0x1b);
    LOG("apic base: %p\n",apic_base & ~(0xfff));
    LOG("bsp: %d\n",(apic_base >> 8) & 1);
    LOG("apic global: %d\n",(apic_base >> 11) & 1);
    //LOG("x2apic enable: %d\n",(apic_base >> 10) & 1);
    mb();
    printk("bsp apic id is %x\n",*(volatile u32 *)ICR_LOW);
    mb();

}



