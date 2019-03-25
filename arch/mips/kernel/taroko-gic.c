/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2008 Ralf Baechle (ralf@linux-mips.org)
 * Copyright (C) 2012 MIPS Technologies, Inc.  All rights reserved.
 * Copyright 2012  Tony Wu (tonywu@realtek.com)
 */
#include <linux/bitmap.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/irq.h>
#include <linux/hardirq.h>
#include <linux/clocksource.h>

#include <asm-generic/bitops/find.h>

#include <asm/io.h>
#include <asm/taroko-mmcr.h>
#include <asm/taroko-gic.h>
#include <asm/setup.h>
#include <asm/traps.h>

static unsigned int gic_irq_flags[GIC_NUM_INTRS];

struct gic_pcpu_mask {
	DECLARE_BITMAP(pcpu_mask, GIC_NUM_INTRS);
};

struct gic_pending_regs {
	DECLARE_BITMAP(pending, GIC_NUM_INTRS);
};

struct gic_intrmask_regs {
	DECLARE_BITMAP(intrmask, GIC_NUM_INTRS);
};

static struct gic_pcpu_mask pcpu_masks[NR_CPUS];
static struct gic_pending_regs pending_regs[NR_CPUS];
static struct gic_intrmask_regs intrmask_regs[NR_CPUS];

unsigned int plat_ipi_resched_int_xlate(unsigned int cpu)
{
	return GIC_IPI_RESCHED(cpu);
}

unsigned int plat_ipi_call_int_xlate(unsigned int cpu)
{
	return GIC_IPI_CALL(cpu);
}

void gic_send_ipi(unsigned int intr)
{
	pr_debug("CPU%d: %s status %08x\n", smp_processor_id(), __func__,
		 read_c0_status());
	GIC_SET_IPI_MASK(intr);
}

static void __init gic_setup_local(unsigned int numcore, unsigned int flag, unsigned int intr)
{
	int i;

	flag &= GIC_FLAG_LOCAL_MASK;

	/* Setup the timer interrupts for all cores */
	for (i = 0; i < numcore; i++) {
		switch (flag) {
		case GIC_FLAG_TIMER:
			GIC_REG32p(SITIMER_R2P, i*4) = intr;
			break;
		}
	}
}

/* Get GIC pending intr number */
int gic_get_int(void)
{
	unsigned int i;
	unsigned long *pending, *intrmask, *pcpumask;

	/* Get per-cpu bitmaps */
	pending = pending_regs[smp_processor_id()].pending;
	intrmask = intrmask_regs[smp_processor_id()].intrmask;
	pcpumask = pcpu_masks[smp_processor_id()].pcpu_mask;

	for (i = 0; i < BITS_TO_LONGS(GIC_NUM_INTRS); i++) {
		pending[i] = GIC_REG32p(PEND, i*4);
		intrmask[i] = GIC_REG32p(MASK, i*4);
	}

	bitmap_and(pending, pending, intrmask, GIC_NUM_INTRS);
	bitmap_and(pending, pending, pcpumask, GIC_NUM_INTRS);

	/* Find first set bit */
	i = find_first_bit(pending, GIC_NUM_INTRS);
	if (i == GIC_NUM_INTRS)
		return -1;

	pr_debug("CPU%d: %s pend=%d\n", smp_processor_id(), __func__, i);

	return MIPS_GIC_IRQ_BASE + i;
}

static void gic_ack_irq(struct irq_data *d)
{
	unsigned int intr = d->irq - MIPS_GIC_IRQ_BASE;

	pr_debug("CPU%d: %s: irq%d\n", smp_processor_id(), __func__, intr);
	GIC_CLR_INTR_MASK(intr);

	/* if it's IPI, clear IPI mask */
	if (gic_irq_flags[intr] & GIC_FLAG_IPI)
		GIC_CLR_IPI_MASK(intr);
}

static void gic_mask_irq(struct irq_data *d)
{
	GIC_CLR_INTR_MASK(d->irq - MIPS_GIC_IRQ_BASE);
}

static void gic_unmask_irq(struct irq_data *d)
{
	GIC_SET_INTR_MASK(d->irq - MIPS_GIC_IRQ_BASE);
}

#ifdef CONFIG_SMP
static DEFINE_SPINLOCK(gic_lock);

static int gic_set_affinity(struct irq_data *d, const struct cpumask *cpumask,
			    bool force)
{
	unsigned int irq = d->irq - MIPS_GIC_IRQ_BASE;
	cpumask_t tmp = CPU_MASK_NONE;
	unsigned long flags;
	int i;

	cpumask_and(&tmp, cpumask, cpu_online_mask);
	if (cpus_empty(tmp))
		return -1;

	/* Assumption : cpumask refers to a single CPU */
	spin_lock_irqsave(&gic_lock, flags);

	/* Re-route this IRQ */
	GIC_ROUTE_TO_CORE(irq, first_cpu(tmp));

	/* Update the pcpu_masks */
	for (i = 0; i < NR_CPUS; i++)
		clear_bit(irq, pcpu_masks[i].pcpu_mask);
	set_bit(irq, pcpu_masks[first_cpu(tmp)].pcpu_mask);

	cpumask_copy(d->affinity, cpumask);
	spin_unlock_irqrestore(&gic_lock, flags);

	return IRQ_SET_MASK_OK_NOCOPY;
}
#endif

static struct irq_chip gic_irq_controller = {
	.name			=	"TAROKO GIC",
	.irq_ack		=	gic_ack_irq,
	.irq_mask		=	gic_mask_irq,
	.irq_mask_ack		=	gic_mask_irq,
	.irq_unmask		=	gic_unmask_irq,
	.irq_eoi		=	gic_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	=	gic_set_affinity,
#endif
};

static void __init gic_setup_intr(unsigned int intr, unsigned int cpu,
				  unsigned int pin, unsigned int flags)
{
	/* Setup Intr to PIN mapping */
	GIC_ROUTE_TO_PIN(intr, pin);

	/* Setup Intr to CPU mapping */
	GIC_ROUTE_TO_CORE(intr, cpu);

	/* Init Intr Masks */
	GIC_CLR_INTR_MASK(intr);

	/* Initialise per-cpu Interrupt software masks */
	set_bit(intr, pcpu_masks[cpu].pcpu_mask);

	if (flags & GIC_FLAG_IPI) {
		GIC_CLR_IPI_MASK(intr);
		set_bit(intr, pcpu_masks[cpu].pcpu_mask);
	}

	gic_irq_flags[intr] = flags;
}

/*
 * Setup IPI interrupts
 *
 * IPI resched interrupts are appeneded to gic_intr_map starting at
 * GIC_NUM_INTRS - NR_CPUS * 2
 *
 * IPI call interrupts are appened to gic_intr_map starting at
 * GIC_NUM_INTRS - NR_CPUS
 *
 * i.e.
 *
 * cpu0     resched    GIC_NUM_INTRS - NR_CPUS * 2
 * cpu1     resched
 * cpu2     resched
 * cpu3     resched
 * cpu0     call       GIC_NUM_INTRS - NR_CPUS
 * cpu1     call
 * cpu2     call
 * cpu3     call
 */
void __init gic_setup_ipi(unsigned int resched, unsigned int call)
{
	int cpu;

	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		gic_setup_intr(GIC_IPI_RESCHED(cpu), cpu, resched, GIC_FLAG_IPI);
		gic_setup_intr(GIC_IPI_CALL(cpu), cpu, call, GIC_FLAG_IPI);
	}
}

void __init gic_init(struct gic_intr_map *intrmap)
{
	unsigned int i, cpu, irq;
	int numcore, numintr;

	numcore = MMCR_VAL32(CORE, NUMCORES);
	numintr = MMCR_VAL32(CORE, NUMINTRS);

	/* Setup defaults */
	for (i = 0; i < numintr; i++) {
		GIC_CLR_INTR_MASK(i);
		if (i < GIC_NUM_INTRS)
			gic_irq_flags[i] = 0;
	}

	/* Setup specifics */
	for (i = 0; i < GIC_NUM_INTRS; i++) {
		cpu = intrmap[i].cpunum;
		irq = MIPS_GIC_IRQ_BASE + i;

		if (intrmap[i].flags & GIC_FLAG_LOCAL_MASK) {
			gic_setup_local(numcore, intrmap[i].flags, intrmap[i].pin);
			continue;
		}

		irq_set_chip(irq, &gic_irq_controller);

		if (cpu == GIC_UNUSED)
			continue;

		gic_setup_intr(i,
			       intrmap[i].cpunum,
			       intrmap[i].pin,
			       intrmap[i].flags);
		irq_set_handler(irq, handle_level_irq);
	}
}
