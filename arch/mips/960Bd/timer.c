/*
 * Realtek Semiconductor Corp.
 *
 * bsp/timer.c
 *     bsp timer initialization code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/time.h>
#include "bspchip.h"

#if (defined(CONFIG_SMP) && (RTK_CEVT_TIMER_NR < CONFIG_NR_CPUS))
#	error "EE: RTK_CEVT_TIMER_NR < CONFIG_NR_CPUS"
#endif

DEFINE_PER_CPU(struct clock_event_device, rtk_clockevent);

static int
rtk_timer_set_next_event(unsigned long delta, struct clock_event_device *evt) {
  return -EINVAL;
}

static void
rtk_timer_set_mode(enum clock_event_mode mode, struct clock_event_device *evt) {
  return;
}

static void
rtk_timer_event_handler(struct clock_event_device *dev) {
	return;
}

static irqreturn_t
rtk_timer_interrupt(int irq, void *dev_id) {
	struct clock_event_device *cd;
	const int cpu = smp_processor_id();
	const uint32_t tcint = timer_base[cpu] + 12;

	REG32(tcint) |= (1 << TC_IP_O);

	cd = &per_cpu(rtk_clockevent, cpu);
	cd->event_handler(cd);

	return IRQ_HANDLED;
}

static struct irqaction rtk_irqaction = {
	.handler = rtk_timer_interrupt,
	.flags = IRQF_PERCPU | IRQF_TIMER,
	.name = "RTK Timer",
};

void _rtk_time_init(const int cpu_idx) {
	const uint32_t tcdata = timer_base[cpu_idx];
	const uint32_t tccount = tcdata + 4;
	const uint32_t tccntl = tcdata + 8;
	const uint32_t tcint = tcdata + 12;

	struct clock_event_device *cd = &per_cpu(rtk_clockevent, cpu_idx);

	REG32(tccntl) = 0;
	REG32(tcint) = (1 << TC_IP_O);
	REG32(tcdata) = (1000*1000)/CONFIG_HZ;
	REG32(tccount) = 0;
	REG32(tcint) = (1 << TC_IE_O);

	cd->name = "RTK Timer";
	cd->features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_PERCPU;
	cd->set_next_event = rtk_timer_set_next_event;
	cd->set_mode = rtk_timer_set_mode;
	cd->event_handler = rtk_timer_event_handler;
	cd->irq = timer_irq[cpu_idx];

	clockevent_set_clock(cd, CONFIG_HZ);
	cd->rating = 100;
	cd->max_delta_ns = clockevent_delta2ns(0x7fffffff, cd);
	cd->min_delta_ns = clockevent_delta2ns(0x300, cd);
	cd->cpumask = cpumask_of(cpu_idx);

	clockevents_register_device(cd);
	setup_irq(timer_irq[cpu_idx], &rtk_irqaction);
	irq_set_handler(timer_irq[cpu_idx], handle_percpu_irq);

	REG32(tccntl) = (1 << TC_EN_O) | (1 << TC_MODE_O) | (BSP_MHZ); // Make a 1MHz timer

	return;
}

void __init plat_time_init(void) {
	_rtk_time_init(0);

	return;
}
