#include <linux/irq.h>
#include <asm/setup.h>
#include "bspchip.h"

extern void plat_irq_peri_hi1_dispatch(void);
extern void plat_irq_peri_lo1_dispatch(void);
extern void plat_irq_peri_hi0_dispatch(void);
extern void plat_irq_peri_lo0_dispatch(void);
extern void plat_irq_peri_crit_dispatch(void);
extern void plat_timer_handler(void);

void __init rtk_irq_init_board(void) {
	set_vi_handler(RTK_IV_LO0, plat_irq_peri_lo0_dispatch);
	set_vi_handler(RTK_IV_HI0, plat_irq_peri_hi0_dispatch);
	set_vi_handler(RTK_IV_LO1, plat_irq_peri_lo1_dispatch);
	set_vi_handler(RTK_IV_HI1, plat_irq_peri_hi1_dispatch);
	set_vi_handler(RTK_IV_CRIT, plat_irq_peri_crit_dispatch);
	set_vi_handler(RTK_IV_TIMER, plat_timer_handler);

	return;
}
