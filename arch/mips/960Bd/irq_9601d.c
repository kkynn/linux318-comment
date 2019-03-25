#include <linux/irq.h>
#include <asm/setup.h>
#include "bspchip.h"

extern void _rtk_irq_dispatch(const int irq_start, const int iv);
extern rtk_intc_mgmt_t intc_mgmt[6];
extern void plat_irq_peri_hi1_dispatch(void);
extern void plat_irq_peri_lo1_dispatch(void);
extern void plat_irq_peri_hi0_dispatch(void);
extern void plat_irq_peri_lo0_dispatch(void);

void __init rtk_irq_init_board(void) {
	return;
}

void plat_irq_dispatch(void) {
	rtk_intc_mgmt_t *intc = &(per_cpu(intc_mgmt, smp_processor_id())[RTK_IV_TIMER-2]);
	uint32_t iid, iid_mask;

	plat_irq_peri_hi1_dispatch();
	plat_irq_peri_lo1_dispatch();
	plat_irq_peri_hi0_dispatch();
	plat_irq_peri_lo0_dispatch();

	/* For timer int. dispatch */
	intc->pending |= (REG32(intc->gimr_addr) & REG32(intc->gimr_addr + 8));
	intc->pending &= intc->mask;
	for (iid=INT_TC0; iid<(INT_TC5+1); iid++) {
		iid_mask = 1 << (iid % 32);
		if (intc->pending & iid_mask) {
			intc->pending &= (~iid_mask);
			do_IRQ(iid);
			break;
		}
	}
	return;
}
