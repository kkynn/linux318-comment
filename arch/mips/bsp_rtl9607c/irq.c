/*
 * Realtek Semiconductor Corp.
 *
 * bsp/irq.c
 *     bsp interrupt initialization and handler code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel_stat.h>
#include <linux/kernel.h>
#include <linux/random.h>
#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <asm/gic.h>
#include "bspchip.h"


/***************************************************
 *  GIC Interrupt External Source
 * ************************************************/
/*
Index i	Interrupt Source	Description
0.		TC0	Timer/Counter #0 interrupt
1.		TC1	Timer/Counter #1 interrupt
2.		TC2	Timer/Counter #2 interrupt
3.		TC3	Timer/Counter #3 interrupt
4.		TC4	Timer/Counter #4 interrupt
5.		TC5	Timer/Counter #5 interrupt
6.		TC6	Timer/Counter #6 interrupt
7.		TC7	Timer/Counter #7 interrupt
8.		TC8	Timer/Counter #8 interrupt
9.		PCM0	PCM0 interrupt
10.		PCM1	PCM1 interrupt
11.		VOIPXSI	VOIP XSI interrupt
12.		VOIPSPI	SPI for VoIP SLIC interrupt
13.		VOIPACC	VOIPACC interrupt
14.		FFTACC	FFTACC interrupt
15.		SWCORE	Switch Core interrupt
16.		GMAC0	GMAC#0 interrupt
17.		GMAC1	GMAC#1 interrupt
18.		GMAC2	GMAC#2 interrupt
19.		EXT_GPIO0	Ext. GPIO 0 interrupt
20.		EXT_GPIO1	Ext. GPIO 1 interrupt
21.		GPIO_ABCD	GPIO_ABCD interrupt
22.		GPIO_EFGH	GPIO_EFGH interrupt
23.		UART0 	UART0 interrupt
24.		UART1	UART1 interrupt
25.		UART2	UART2 interrupt
26.		UART3	UART3 interrupt
27.		TC0_DELAY_INT	Delayed interrupt with Timer/Counter 0
28.		TC1_DELAY_INT	Delayed interrupt with Timer/Counter 1
29.		TC2_DELAY_INT	Delayed interrupt with Timer/Counter 2
30.		TC3_DELAY_INT	Delayed interrupt with Timer/Counter 3
31.		TC4_DELAY_INT	Delayed interrupt with Timer/Counter 4
32.		TC5_DELAY_INT	Delayed interrupt with Timer/Counter 5
33.		WDT_PH1TO	Watchdog Timer Phase 1 Timeout interrupt
34.		WDT_PH2TO	Watchdog Timer Phase 2 Timeout interrupt
35.		OCP0TO	OCP0 bus time-out interrupt
36.		OCP1TO	OCP1 bus time-out interrupt
37.		LXPBOTO	LX PBO bus time-out interrupt
38.		LXMTO	LX Master 0/1/2/3 time-out interrupt
39.		LXSTO	LX Slave 0/1/2/3/P time-out interrupt
40.		OCP0_ILA	OCP0 illegal addr. interrupt
41.		OCP1_ILA	OCP1 illegal addr. interrupt
42.		PONNIC	PONNIC interrupt
43.		PONNIC_DS	PONNIC_DS interrupt
44.		BTG	BTG/GDMA interrupts from LX0/1/2/3 & PBO_D(U)S_R(W)
45.		CPI_P1	Cross-Processor interrupt P1
46.		CPI_P2	Cross-Processor interrupt P2
47.		CPI_P3	Cross-Processor interrupt P3
48.		CPI_P4	Cross-Processor interrupt P4
49.		PCIE0	PCIE0 interrupt
50.		PCIE1	PCIE1 interrupt
51.		USB_2	USB 2.0 interrupt
52.		USB_23	USB 2/3 interrupt
53.		P_NAND	Parallel NAND flash DMA interrupt
54.		SPI_NAND	SPI-NAND DMA interrupt
55.		ECC	ECC Controller interrupt
56.		IPI0	Reserved for MIPS inter-processor interrupt #0
57.		IPI1	Reserved for MIPS inter-processor interrupt #1
58.		IPI2	Reserved for MIPS inter-processor interrupt #2
59.		IPI3	Reserved for MIPS inter-processor interrupt #3
60.		IPI4	Reserved for MIPS inter-processor interrupt #4
61.		IPI5	Reserved for MIPS inter-processor interrupt #5
62.		IPI6	Reserved for MIPS inter-processor interrupt #6
63.		IPI7	Reserved for MIPS inter-processor interrupt #7
*/

/*
 * MIPS 1004k/IA Interrupt Scheme
 *   GIC_FLAG_TRANSPARENT : The GIS MASK is set,(IRQ ENABLE).
 *                          irq_request/setup_irq also set GIS MASK.
 * 
 *   Source     EXT_INT   IRQ      PIC Out  GIC_Out     CPU_CAUSE
 *                                 (GIC IN)
 *   --------   -------   ------   ------- ------------   -------- 
 *  VPE Num --- VPE's IP2~IP7 ---  Interrupt Polarity --- Trigger Type -- FLAG
********************************************************************************/
#define GIC_CPU_NMI GIC_MAP_TO_NMI_MSK
#define X GIC_UNUSED
static struct gic_intr_map bsp_gic_intr_map[GIC_NUM_INTRS] = {
	{	0,	GIC_CPU_INT5,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT }, /*IRQ0,  timer 0 -> VPE0 */
	{	1,	GIC_CPU_INT5,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT }, /*IRQ1,  timer 1 -> VPE1 */
	{	2,	GIC_CPU_INT5,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT }, /*IRQ2,  timer 2 -> VPE2 */
	{	3,	GIC_CPU_INT5,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT }, /*IRQ3,  timer 3 -> VPE3 */
	{	X,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	X,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT }, /*IRQ6,  timer 6 -> VPE0 */
	{	X,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 

	{	X,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	X,	X,	X,	X,	0 }, 	// PCM0
/* 10 */
	{	X,	X,	X,	X,	0 }, 	// PCM1
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
/* 20 */
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT2,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	GIC_FLAG_TRANSPARENT },  /*23, UART0 */
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
/* 30 */
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 },
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 },
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 },
/* 40 */
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 },
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 },
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT0,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
/* 50 */
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
	{	0,	GIC_CPU_INT1,	GIC_POL_POS,	GIC_TRIG_LEVEL,	0 }, 
/* 56 ~ 63 : Reserved for SMP IPI. 8 = (The number of VPE)x2  */	
	
};


#undef X
/*GIC_NUM_INTRS bit map */

/*GIC_CPU_INT2, IRQ23~48*/
/*Priority: LOW */
DECLARE_BITMAP(bsp_intsIP2, GIC_NUM_INTRS);

/*GIC_CPU_INT3, IRQ49~55*/
DECLARE_BITMAP(bsp_intsIP3, GIC_NUM_INTRS);

/*GIC_CPU_INT1, IRQ9~22*/
/*Priority: HIGH */
DECLARE_BITMAP(bsp_intsIP4, GIC_NUM_INTRS);


static void __init fill_bsp_int_map(void){
  
  /* IP2 */
  bitmap_set(bsp_intsIP2, 6, 6);   //IRQ6
  bitmap_set(bsp_intsIP2, 23, 26); //IRQ23~48

  /* IP3 */  
  bitmap_set(bsp_intsIP3, 49, 7);  //IRQ49~IRQ55
  
  /* IP4 */
  bitmap_set(bsp_intsIP4, 9, 14);  //IRQ9~IRQ22
  
}

static inline void bsp_get_gic_int_mask(unsigned long *map){
    	unsigned long irq;
	DECLARE_BITMAP(pending, GIC_NUM_INTRS);
	gic_get_int_mask(pending, map);
	irq = find_first_bit(pending, GIC_NUM_INTRS);
	while (irq < GIC_NUM_INTRS) {
		do_IRQ(MIPS_GIC_IRQ_BASE + irq);
		irq = find_next_bit(pending, GIC_NUM_INTRS, irq + 1);
	}
}
/****************************************************
 * IPI
 * *************************************************/
#ifdef CONFIG_SMP
DECLARE_BITMAP(ipi_ints, GIC_NUM_INTRS);
static inline void bsp_ipi_irqdispatch(void)
{
	unsigned long irq;
	DECLARE_BITMAP(pending, GIC_NUM_INTRS);

	gic_get_int_mask(pending, ipi_ints);

	irq = find_first_bit(pending, GIC_NUM_INTRS);

	while (irq < GIC_NUM_INTRS) {
		do_IRQ(MIPS_GIC_IRQ_BASE + irq);

		irq = find_next_bit(pending, GIC_NUM_INTRS, irq + 1);
	}

}

static void __init fill_ipi_map1(int baseintr, int cpu, int cpupin)
{
	int intr = baseintr + cpu;
	bsp_gic_intr_map[intr].cpunum = cpu;
	bsp_gic_intr_map[intr].pin = cpupin;
	bsp_gic_intr_map[intr].polarity = GIC_POL_POS;
	bsp_gic_intr_map[intr].trigtype = GIC_TRIG_EDGE;
	bsp_gic_intr_map[intr].flags = 0;
        //ipi_map[cpu] |= (1 << (cpupin + 2));
	bitmap_set(ipi_ints, intr, 1);
}

static void __init fill_ipi_map(void)
{
	int cpu;
	for (cpu = 0; cpu < nr_cpu_ids; cpu++) {
		fill_ipi_map1(GIC_IPI_RESCHED_BASE, cpu, GIC_CPU_INT3);
		fill_ipi_map1(GIC_IPI_CALL_BASE, cpu, GIC_CPU_INT4);
	}
}

static void __init irq_validity_check(void) {
	int i;
	struct gic_intr_map *gic;
	for (i=0; i<ARRAY_SIZE(bsp_gic_intr_map); i++) {
		gic = &bsp_gic_intr_map[i];
		if (GIC_UNUSED == gic->cpunum)
			continue;
		if ((gic->cpunum+1) > CONFIG_NR_CPUS) {
			gic->cpunum = GIC_UNUSED;
			printk("Correct IRQ entries %d\n", i);
		}			
	}
}
#endif


/**************************************************
 * call gic_init of arch/mips/irq_gic.c on VPE0
 * at boottime
**************************************************/
void __init init_gic(void){
#ifdef CONFIG_SMP
	irq_validity_check();
	fill_ipi_map();
#endif
	fill_bsp_int_map();
	gic_init(GIC_BASE_ADDR, GIC_ADDRSPACE_SZ, bsp_gic_intr_map);
	printk("gic_init: GIC_BASE_ADDR=0x%08x, GIC_ADDRSPACE_SZ=0x%08x\n", GIC_BASE_ADDR, GIC_ADDRSPACE_SZ);

}
/******************************************************
 * GIC disable routing local timer and perf intterrupt
 * ****************************************************/
void __init vpe_local_disable(void)
{
	unsigned int vpe_ctl;
	
	/* Are Interrupts locally routable? */
	GICREAD(GIC_REG(VPE_LOCAL, GIC_VPE_CTL), vpe_ctl);
	if (vpe_ctl & GIC_VPE_CTL_TIMER_RTBL_MSK)
			GICWRITE(GIC_REG(VPE_LOCAL, GIC_VPE_TIMER_MAP), 0);

	if (vpe_ctl & GIC_VPE_CTL_PERFCNT_RTBL_MSK)
			GICWRITE(GIC_REG(VPE_LOCAL, GIC_VPE_PERFCTR_MAP), 0);

}



/***************************************************
 * If CCONFIG_SMP, We use the GIC to implement IPI
 * 
 ***************************************************/
#ifdef CONFIG_SMP
static irqreturn_t ipi_resched_interrupt(int irq, void *dev_id)
{
	/*From linux-3.x add this function */
	scheduler_ipi();
	return IRQ_HANDLED;
}

static irqreturn_t ipi_call_interrupt(int irq, void *dev_id)
{
	smp_call_function_interrupt();
	return IRQ_HANDLED;
}

static struct irqaction irq_resched = {
	.handler	= ipi_resched_interrupt,
	.flags		= IRQF_PERCPU,
	.name		= "IPI_resched"
};

static struct irqaction irq_call = {
	.handler	= ipi_call_interrupt,
	.flags		= IRQF_PERCPU,
	.name		= "IPI_call"
};
static void __init arch_init_ipiirq(int irq, struct irqaction *action)
{
	setup_irq(irq, action);
	irq_set_handler(irq, handle_percpu_irq);
}


#endif

#ifndef CONFIG_CPU_MIPSR2_IRQ_VI
#error "Must select CONFIG_CPU_MIPSR2_IRQ_VI for MIPS IA !!!"
#endif
/************************************
 * Vector0 & vector1 are for SW0/W1
 * Relative Interrupt Priority for Vectored Interrupt Mode
-----------------------------------------------
Highest Priority | Hardware | HW5 IP7 and IM7 7
                 |          | HW4 IP6 and IM6 6
                 |          | HW3 IP5 and IM5 5
                 |          | HW2 IP4 and IM4 4
                 |          | HW1 IP3 and IM3 3
                 |          | HW0 IP2 and IM2 2
-----------------------------------------------
                 | Software | SW1 IP1 and IM1 1
Lowest Priority  |          | SW0 IP0 and IM0 0
-----------------------------------------------
************************************/
/* Vector2 */
void bsp_ictl_irq_dispatch_v2(void)
{	
	bsp_get_gic_int_mask(bsp_intsIP2);
}

/* Vector3*/
void bsp_ictl_irq_dispatch_v3(void)
{	
	bsp_get_gic_int_mask(bsp_intsIP3);
}
/* Vector4 */
void bsp_ictl_irq_dispatch_v4(void)
{	
        bsp_get_gic_int_mask(bsp_intsIP4);
}

/**************************************************
 * Vector5,6
 * IPI
 * IPI interrupts are installed at the end of gic_intr_map
 * shown as follows:
 *
 * cpu0		resched		GIC_NUM_INTRS - NR_CPUS * 2
 * cpu1		resched
 * cpu2		resched
 * cpu3		resched
 * cpu0		call		GIC_NUM_INTRS - NR_CPUS
 * cpu1		call
 * cpu2		call
 * cpu3		call
 *************************************************/
#ifdef CONFIG_SMP
/* Vector5 */
void bsp_ictl_irq_dispatch_v5(void)
{	
	bsp_ipi_irqdispatch();
}
/* Vector6 */
void bsp_ictl_irq_dispatch_v6(void)
{	
	bsp_ipi_irqdispatch();
}
#endif
/**************************************************
 * Vector7
 * Timer
 *************************************************/

#ifdef CONFIG_CEVT_EXT
void bsp_ictl_irq_dispatch_timer(void)
{	
#ifdef CONFIG_SMP
  int cpu = smp_processor_id();
  do_IRQ(BSP_TC0_IRQ + cpu );	
#else  
  do_IRQ(BSP_TC0_IRQ);	
#endif

}
#endif

/**************************************************************************************
 * CPU_MIPSR2_IRQ_VI only support 8 vectored interrupt 
 * corresponded  with IP0 ~ IP7 
 * Vector 0 for IP0(SW0)
 * Vector 1 for IP1(SW1)
 * 
 * 
 * 
 * Vector 5 for IP5  ---> IPI resched
 * Vector 6 for IP6  ---> IPI call
 * Vector 7 for IP7  ---> External Timer(CEVT_EXT),(MIPS Default, used by CEVT_R4K)
 *************************************************************************************/
static void __init  bsp_setup_vi(void){
/* IP2 */
	set_vi_handler(2, bsp_ictl_irq_dispatch_v2);
/* IP3 */
	set_vi_handler(3, bsp_ictl_irq_dispatch_v3);
/* IP4 */
	set_vi_handler(4, bsp_ictl_irq_dispatch_v4);
#ifdef CONFIG_SMP
/* This is for LINUX SMP Inter-Processor Interrupts */
/* IP5,  IPI, Vector 5 */
	set_vi_handler(5, bsp_ictl_irq_dispatch_v5);
/* IP6,  IPI, Vector 6 */
	set_vi_handler(6, bsp_ictl_irq_dispatch_v6);
#endif
	
#ifdef CONFIG_CEVT_EXT	
	set_vi_handler(7, bsp_ictl_irq_dispatch_timer);         /*IP7,  Timer, Vector 7 */
#endif

			
}


static void __init bsp_irq_init(void)
{
#ifdef CONFIG_SMP
   int i;
#endif
   
#ifdef CONFIG_CEVT_EXT
    vpe_local_disable();
   /* Do not disable count, because linux-3.x will use count vaule do somethime.TODO Why and where */
   /* Disable internal Count register */
   //   write_c0_cause(read_c0_cause() | (1 << 27));

   /* Clear internal timer interrupt */
   write_c0_compare(0);
   
#endif
   /* Enable all interrupt mask of CPU */
    change_c0_status(ST0_IM, STATUSF_IP2 | STATUSF_IP3 |  STATUSF_IP4 |
				STATUSF_IP5 | STATUSF_IP6 | STATUSF_IP7 );

   
#ifdef CONFIG_SMP
/* GIC is mandatory */
   for (i = 0; i < nr_cpu_ids; i++) {
	arch_init_ipiirq(MIPS_GIC_IRQ_BASE +
					GIC_IPI_RESCHED(i), &irq_resched);
	arch_init_ipiirq(MIPS_GIC_IRQ_BASE +
					 GIC_IPI_CALL(i), &irq_call);
  }
#endif
}

void __init plat_irq_init(void)
{
#ifdef CONFIG_CEVT_R4K
	mips_cpu_irq_init();
#endif
#ifdef CONFIG_CPU_MIPSR2_IRQ_VI
 	bsp_setup_vi();
#endif	

	/* Initialize GIC before PIC */
	init_gic();

	/* initialize IRQ action handlers */
	bsp_irq_init();
}
