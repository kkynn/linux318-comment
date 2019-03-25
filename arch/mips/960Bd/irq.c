/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.c
 *     bsp interrupt initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <asm/setup.h>
#include "bspchip.h"

extern void plat_smp_intc_init(void);
extern void rtk_irq_init_board(void);

DEFINE_SPINLOCK(int_cntlr0_lock);

/* 6 for RTK_IV_LO0, RTK_IV_HI0, RTK_IV_LO1, RTK_IV_HI1,
   RTK_IV_CRIT, and RTK_IV_TIMER respectively. */
DEFINE_PER_CPU(typeof(rtk_intc_mgmt_t)[6], intc_mgmt);

static void mask_rtk_irq(struct irq_data *d) {
	const int cpu_off = 0 * 0x40;
	unsigned long flags;

	spin_lock_irqsave(&int_cntlr0_lock, flags);
	REG_GIMR(cpu_off, d->irq/32) &= (~(1 << (d->irq%32)));
	spin_unlock_irqrestore(&int_cntlr0_lock, flags);

	return;
}

static void unmask_rtk_irq(struct irq_data *d) {
	const int cpu_off = 0 * 0x40;
	unsigned long flags;

	spin_lock_irqsave(&int_cntlr0_lock, flags);
	REG_GIMR(cpu_off, d->irq/32) |= (1 << (d->irq%32));
	spin_unlock_irqrestore(&int_cntlr0_lock, flags);

	return;
}

static struct irq_chip bsp_intc_cpu0 = {
	.name = "IntC 0",
	.irq_ack = mask_rtk_irq,
	.irq_mask = mask_rtk_irq,
	.irq_mask_ack = mask_rtk_irq,
	.irq_unmask = unmask_rtk_irq,
	.irq_eoi = unmask_rtk_irq,
	.irq_disable = mask_rtk_irq,
	.irq_enable = unmask_rtk_irq,
};

static void
_rtk_irq_dispatch(const int irq_start, const int iv) {
	rtk_intc_mgmt_t *intc = &(per_cpu(intc_mgmt, smp_processor_id())[iv-2]);
	uint32_t iid, iid_mask;

	intc->pending |= (REG32(intc->gimr_addr) & REG32(intc->gimr_addr + 8));
	intc->pending &= intc->mask;

	for (iid=irq_start; iid<(irq_start+32); iid++) {
		iid_mask = 1 << (iid % 32);
		if (intc->pending & iid_mask) {
			intc->pending &= (~iid_mask);
			do_IRQ(iid);
			break;
		}
	}

	return;
}

void plat_irq_peri_lo1_dispatch(void) {
	_rtk_irq_dispatch(32, RTK_IV_LO1);
	return;
}

void plat_irq_peri_hi1_dispatch(void) {
	_rtk_irq_dispatch(0, RTK_IV_HI1);
	return;
}

void plat_irq_peri_lo0_dispatch(void) {
	_rtk_irq_dispatch(32, RTK_IV_LO0);
	return;
}

void plat_irq_peri_hi0_dispatch(void) {
	_rtk_irq_dispatch(0, RTK_IV_HI0);
	return;
}

/* This dispatcher is designed for critical IRQs that have their own dispatcher flow. */
void plat_irq_peri_crit_dispatch(void) {
	printk("DD: Crit. dispatcher is empty\n");

	return;
}

void plat_timer_handler(void) {
	do_IRQ(timer_irq[smp_processor_id()]);
	return;
}

void rtk_cp0_interrupt_init(void) {
	prom_printf("II: Initializing CPU%d CP0 STATUS/CAUSE/COMPARE...\n", smp_processor_id());
	clear_c0_status(ST0_IM);
	clear_c0_cause(CAUSEF_IP);
	write_c0_compare(0);
	change_c0_status(ST0_IM,
	                 STATUSF_IP0 | STATUSF_IP1 | STATUSF_IP2 | STATUSF_IP3 |
	                 STATUSF_IP4 | STATUSF_IP5 | STATUSF_IP6 | STATUSF_IP7);
	return;
}

void __init rtk_intc_init(const int cpu) {
	const int cpu_off = cpu*0x40; //This is platform-dependant, 0x40 is for 9603d(0921).
	int i;
	rtk_intc_mgmt_t *intc = &(per_cpu(intc_mgmt, cpu)[0]);

	prom_printf("II: Initializing IntC%d...\n", cpu);

	REG_GIMR(cpu_off, 0) = (1 << INT_PERIPHERAL);
	REG_GIMR(cpu_off, 1) = 0;

	for (i=0; i<7; i++) {
		REG_GIRR(cpu_off, i) = 0;
	}

	for (i=0; i<6; i++) {
		intc[i].gimr_addr = BSP_INTC_BASE + cpu_off + (((i+1)%2)*4);
		intc[i].pending = 0;
		intc[i].mask = 0;
	}
	intc[RTK_IV_CRIT-2].gimr_addr = 0;
	intc[RTK_IV_TIMER-2].gimr_addr = intc[RTK_IV_LO1-2].gimr_addr;

	return;
}

void __init rtk_intc_setup(uint32_t irq, uint32_t cpu, uint32_t iv) {
	const int cpu_off = cpu*0x40; //This is platform-dependant, 0x40 is for 9603d(0921).
	const uint8_t irr = irq_irr_map[irq];
	rtk_intc_mgmt_t *intc = &(per_cpu(intc_mgmt, cpu)[iv-2]);
	int i;

	prom_printf("II: Binding IRQ%d to CPU%d...\n", irq, cpu);

	if ((irq < 32) && (irq > 1) && ((iv == RTK_IV_HI0) || (iv == RTK_IV_HI0))) {
		goto irq_setup_cont;
	}
	if ((irq > 31) && ((iv == RTK_IV_LO0) || (iv == RTK_IV_LO1))) {
		goto irq_setup_cont;
	}
	if (iv == RTK_IV_TIMER) {
		for (i=0; i<CONFIG_NR_CPUS; i++) {
			if (irq == timer_irq[i]) goto irq_setup_cont;
		}
	}

	prom_printf("EE: Invalid IRQ assignment:\n");
	prom_printf("\tIRQ 00~01: reserved for IPI\n");
	prom_printf("\tIRQ 02~31: RTK_IV_HI0 | RTK_IV_HI1\n");
	prom_printf("\tIRQ 32~63: RTK_IV_LO0 | RTK_IV_LO1\n");
	prom_printf("Except that RTK Timer(s) IRQ");
	for (i=0; i<CONFIG_NR_CPUS; i++) {
		prom_printf(" %d,", timer_irq[i]);
	}
	prom_printf("\b can be with RTK_IV_TIMER\n");

	while (1);

irq_setup_cont:
	REG_GIMR(cpu_off, (irq/32)) |= (1 << (irq%32));
	REG_GIRR(cpu_off, (irr/32)) |= (RTK_MIPS_RS(iv) << (irr%32));

	intc->mask |= (1 << (irq%32));

	return;
}

void __init rtk_irq_init_common(void) {
	int i;

#ifdef CONFIG_SMP
	plat_smp_ipi_init();
	plat_smp_intc_init();
#else
	for (i=0; i<2; i++) {
		irq_set_chip_and_handler(i, &bsp_intc_cpu0, handle_level_irq);
	}
#endif

	rtk_intc_init(0);
	rtk_cp0_interrupt_init();

	/* After rtk_intc_init(*), all corresponded int. cntlr. are reset to zero,
	   and ready for reg. programming. */

	for (i=2; i<INT_NUM; i++) {
		irq_set_chip_and_handler(i, &bsp_intc_cpu0, handle_level_irq);
	}

	rtk_intc_setup(INT_UART0, 0, RTK_IV_LO0);
	rtk_intc_setup(INT_TC0, 0, RTK_IV_TIMER);

	return;
}

void __init plat_irq_init(void) {
	rtk_irq_init_common();
	rtk_irq_init_board();
	return;
}
