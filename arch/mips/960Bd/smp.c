/*
 * Realtek Semiconductor Corp.
 *
 * bsp/vsmp.c
 *     bsp VSMP initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/version.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <asm/setup.h>
#include <asm/smp-ops.h>
#include <asm/irq_cpu.h>

#include "bspchip.h"

void rtk_cp0_interrupt_init(void);
void rtk_intc_init(const int cpu);

DEFINE_SPINLOCK(int_cntlr1_lock);

static void mask_rtk_irq_cpu1(struct irq_data *d) {
	const int cpu_off = 1 * 0x40;
	unsigned long flags;

	spin_lock_irqsave(&int_cntlr1_lock, flags);
	REG_GIMR(cpu_off, d->irq/32) &= (~(1 << d->irq));
	spin_unlock_irqrestore(&int_cntlr1_lock, flags);

	return;
}

static void unmask_rtk_irq_cpu1(struct irq_data *d) {
	const int cpu_off = 1 * 0x40;
	unsigned long flags;

	spin_lock_irqsave(&int_cntlr1_lock, flags);
	REG_GIMR(cpu_off, d->irq/32) |= (1 << d->irq);
	spin_unlock_irqrestore(&int_cntlr1_lock, flags);

	return;
}

struct irq_chip ic_9603d_cpu1 = {
	.name = "IntC 1",
	.irq_ack = mask_rtk_irq_cpu1,
	.irq_mask = mask_rtk_irq_cpu1,
	.irq_mask_ack = mask_rtk_irq_cpu1,
	.irq_unmask = unmask_rtk_irq_cpu1,
	.irq_eoi = unmask_rtk_irq_cpu1,
	.irq_disable = mask_rtk_irq_cpu1,
	.irq_enable = unmask_rtk_irq_cpu1,
};

/* This func. is platform-dependant.
	 Diff. plat. comes with diff. cpu num. and int setting. */
void plat_smp_init_secondary(void) {
	/* For an unknown reason, hoisting _rtk_time_init() causes run-time kernel panic.
	   For now, just leave it here.
	   Fruther remove __init hint of _rtk_time_init() to prevent section mismatch */
	_rtk_time_init(1);
	irq_set_chip(INT_TC1, &ic_9603d_cpu1);

	rtk_cp0_interrupt_init();

	return;
}

static void ipi_resched_dispatch(void) {
	do_IRQ(0);
	return;
}

static void ipi_call_dispatch(void) {
	do_IRQ(1);
	return;
}

static irqreturn_t
ipi_resched_interrupt(int irq, void *dev_id) {
	scheduler_ipi();
	return IRQ_HANDLED;
}

static irqreturn_t
ipi_call_interrupt(int irq, void *dev_id) {
	smp_call_function_interrupt();
	return IRQ_HANDLED;
}

static struct irqaction irq_resched = {
	.handler = ipi_resched_interrupt,
	.flags = IRQF_PERCPU,
	.name = "IPI_resched"
};

static struct irqaction irq_call = {
	.handler = ipi_call_interrupt,
	.flags = IRQF_PERCPU,
	.name = "IPI_call"
};

void __init plat_smp_ipi_init(void) {
	/* Let MIPS sets IRQ 0 ~ 7.
	   In fact we just need 0 & 1 for IPI; will override 2 ~ 7 later. */
	mips_cpu_irq_init();

	set_vi_handler(RTK_IV_IPI_RESCHED, ipi_resched_dispatch);
	setup_irq(INT_IPI_RESCHED, &irq_resched);
	irq_set_handler(INT_IPI_RESCHED, handle_percpu_irq);

	set_vi_handler(RTK_IV_IPI_CALL, ipi_call_dispatch);
	setup_irq(INT_IPI_CALL, &irq_call);
	irq_set_handler(INT_IPI_CALL, handle_percpu_irq);

	return;
}

void __init plat_smp_intc_init(void) {
	rtk_intc_init(1);
	rtk_intc_setup(INT_TC1, 1, RTK_IV_TIMER);
	irq_set_chip(INT_TC1, &ic_9603d_cpu1);

	return;
}

/*
 * Called in bsp/setup.c to initialize SMP operations
 *
 * Depends on SMP type, plat_smp_init calls corresponding
 * SMP operation initializer in arch/mips/kernel
 *
 * Known SMP types are:
 *     MIPS_CMP
 *     MIPS_MT_SMP
 *     MIPS_CMP + MIPS_MT_SMP: 1004K (use MIPS_CMP)
 */
void __init plat_smp_init(void) {
#ifdef  CONFIG_MIPS_CMP
	if (!register_cmp_smp_ops())
		return;
#endif
#ifdef CONFIG_MIPS_MT_SMP
 	if (!register_vsmp_smp_ops())
 		return;
#endif
}
