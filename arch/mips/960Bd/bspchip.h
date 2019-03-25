/*
 * Realtek Semiconductor Corp.
 *
 * bsp/bspchip.h:
 *     bsp chip address and IRQ mapping file
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#ifndef _BSPCHIP_H_
#define _BSPCHIP_H_

#include <linux/init.h>

#if defined(CONFIG_RTL9601D)
#	include <bspchip_9601d.h>
#else
#	include <bspchip_9603d.h>
#endif

#define PROM_DEBUG 0

/*
 * Register access macro
 */
#ifndef REG32
#	define REG32(reg) (*(volatile unsigned int   *)(reg))
#endif

#define REG16(reg) (*(volatile unsigned short *)(reg))
#define REG08(reg) (*(volatile unsigned char  *)(reg))
#define REG8(reg) (*(volatile unsigned char *)((unsigned int)reg))

#define WRITE_MEM32(addr, val) (*(volatile unsigned int *)(addr)) = (val)
#define READ_MEM32(addr) (*(volatile unsigned int *) (addr))
#define WRITE_MEM16(addr, val) (*(volatile unsigned short *)(addr)) = (val)
#define READ_MEM16(addr) (*(volatile unsigned short *)(addr))
#define WRITE_MEM8(addr, val) (*(volatile unsigned char *)(addr)) = (val)
#define READ_MEM8(addr) (*(volatile unsigned char *)(addr))

/* Interrupt */
#define INT_NUM 64

#define INT_IPI_RESCHED 0
#define INT_IPI_CALL 1
#define INT_FFTACC 3
#define INT_ECC 4
#define INT_SPI_NAND 5
#define INT_PONNIC_RXM 6
#define INT_SWITCH 8
#define INT_CPI_P0 9
#define INT_CPI_P1 10
#define INT_CPI_P2 11
#define INT_PERIPHERAL 12
#define INT_USB_2_P1 14
#define INT_PCIE1 16
#define INT_PCM0 17
#define INT_PCM1 18
#define INT_LX_BTG 19
#define INT_PONNIC 24
#define INT_PONNIC_DS 25
#define INT_GMAC0_INT0 26
#define INT_GMAC0_INT1 27
#define INT_XSI 28
#define INT_SPI 29
#define INT_VOIPACC 30
#define INT_TMO 31
#define INT_GPIO_JKMN 32
#define INT_UART_HS_INT 33
#define INT_UART_HS_DMA 34
#define INT_WDT_PH1TO 39
#define INT_WDT_PH2TO 40
#define INT_GPIO_EFGH 41
#define INT_GPIO_ABCD 42
#define INT_TC0 43
#define INT_TC1 44
#define INT_TC2 45
#define INT_TC3 46
#define INT_TC4 47
#define INT_TC5 48
#define INT_UART0 49
#define INT_UART1 50
#define INT_UART2 51
#define INT_UART3 52
#define INT_OCP0_ILA 53
#define INT_OCP1_ILA 54
#define INT_OCPTO0 55
#define INT_OCPTO1 56
#define INT_LBC_TO_SLV 57
#define INT_LBC_TO_MAS 58
#define INT_LBC_PBO_TO_MAS 59
#define INT_TC6 60
#define INT_TC7 61
#define INT_TC8 62

#define REG_GIMR(cpu_off, n) (*(volatile uint32_t *)(BSP_INTC_BASE+(cpu_off)+(n)*4))
#define REG_GISR(cpu_off, n) (*(volatile uint32_t *)(BSP_INTC_BASE+(cpu_off)+(n)*4+0x8))
#define REG_GIRR(cpu_off, n) (*(volatile uint32_t *)(BSP_INTC_BASE+(cpu_off)+(n)*4+0x10))

#define IRR_BOFF(n) (n*32)

const static uint8_t irq_irr_map[INT_NUM] __initconst __attribute__ ((unused)) = {
	[INT_IPI_RESCHED] = 0,
	[INT_IPI_CALL] = 0,
	[INT_FFTACC] = (IRR_BOFF(5) + 8),
	[INT_ECC] = (IRR_BOFF(2) + 4),
	[INT_SPI_NAND] = (IRR_BOFF(2) + 8),
	[INT_PONNIC_RXM] = (IRR_BOFF(2) + 16),
	[INT_SWITCH] = (IRR_BOFF(5) + 24),
	[INT_CPI_P0] = (IRR_BOFF(0) + 28),
	[INT_CPI_P1] = (IRR_BOFF(6) + 24),
	[INT_CPI_P2] = (IRR_BOFF(6) + 28),
	[INT_PERIPHERAL] = 0,
	[INT_USB_2_P1] = (IRR_BOFF(3) + 16),
	[INT_PCIE1] = (IRR_BOFF(3) + 24),
	[INT_PCM0] = (IRR_BOFF(3) + 28),
	[INT_PCM1] = (IRR_BOFF(2) + 0),
	[INT_LX_BTG] = (IRR_BOFF(5) + 16),
	[INT_PONNIC] = (IRR_BOFF(5) + 0),
	[INT_PONNIC_DS] = (IRR_BOFF(5) + 4),
	[INT_GMAC0_INT0] = (IRR_BOFF(1) + 0),
	[INT_GMAC0_INT1] = (IRR_BOFF(1) + 4),
	[INT_XSI] = (IRR_BOFF(1) + 8),
	[INT_SPI] = (IRR_BOFF(1) + 12),
	[INT_VOIPACC] = (IRR_BOFF(1) + 16),
	[INT_GPIO_JKMN] = (IRR_BOFF(5) + 20),
	[INT_UART_HS_INT] = (IRR_BOFF(6) + 16),
	[INT_UART_HS_DMA] = (IRR_BOFF(6) + 20),
	[INT_WDT_PH1TO] = (IRR_BOFF(5) + 12),
	[INT_WDT_PH2TO] = (IRR_BOFF(6) + 12),
	[INT_GPIO_EFGH] = (IRR_BOFF(5) + 28),
	[INT_GPIO_ABCD] = (IRR_BOFF(4) + 0),
	[INT_TC0] = (IRR_BOFF(4) + 4),
	[INT_TC1] = (IRR_BOFF(4) + 8),
	[INT_TC2] = (IRR_BOFF(4) + 12),
	[INT_TC3] = (IRR_BOFF(4) + 16),
	[INT_TC4] = (IRR_BOFF(4) + 20),
	[INT_TC5] = (IRR_BOFF(4) + 24),
	[INT_UART0] = (IRR_BOFF(4) + 28),
	[INT_UART1] = (IRR_BOFF(3) + 0),
	[INT_UART2] = (IRR_BOFF(3) + 4),
	[INT_UART3] = (IRR_BOFF(3) + 8),
	[INT_OCP0_ILA] = (IRR_BOFF(1) + 20),
	[INT_OCP1_ILA] = (IRR_BOFF(1) + 24),
	[INT_OCPTO0] = (IRR_BOFF(1) + 28),
	[INT_OCPTO1] = (IRR_BOFF(0) + 0),
	[INT_LBC_TO_SLV] = (IRR_BOFF(0) + 4),
	[INT_LBC_TO_MAS] = (IRR_BOFF(0) + 8),
	[INT_LBC_PBO_TO_MAS] = (IRR_BOFF(0) + 12),
	[INT_TC6] = (IRR_BOFF(0) + 16),
	[INT_TC7] = (IRR_BOFF(0) + 20),
	[INT_TC8] = (IRR_BOFF(0) + 24),
};

#define RTK_IV_IPI_RESCHED 0
#define RTK_IV_IPI_CALL 1
#define RTK_IV_LO0 2
#define RTK_IV_HI0 3
#define RTK_IV_LO1 4
#define RTK_IV_HI1 5
#define RTK_IV_CRIT 6
#define RTK_IV_TIMER 7

/* Need not record gisr_addr. It can be simply obtained with just an ALU inst., i.e.:
	 lw t0, 0(intc) //get gimr_addr
	 lw t1, 4(intc) //get gisr_addr
	 lw t0, 0(t0) //reg32(gimr_addr)
	 lw t1, 0(t1) //reg32(gisr_addr)
	     v.s.
	 lw t0, 0(intc) //get gimr_addr
	 lw t0, 0(t0) //reg32(gimr_addr)
	 lw t1, 8(t0) //reg32(gimr_addr+8) == reg32(gisr_addr)
 */
typedef struct {
	uint32_t gimr_addr;
	uint32_t mask;
	uint32_t pending;
} rtk_intc_mgmt_t;

/* Timer */
#define TC_BASE (0xb8003200)

#define TC0DATA (TC_BASE + 0x00)
#define TC0CNT (TC_BASE + 0x04)
#define TC0CONTROL (TC_BASE + 0x08)
#	define TC_EN_O (28)
#	define TC_MODE_O (24)
#	define TC_DIV_O (0)
#define TC0INT (TC_BASE + 0x0c)
#	define TC_IE_O (20)
#	define TC_IP_O (16)

#define TC1DATA (TC0DATA + 0x10)
#define TC2DATA (TC0DATA + 0x20)
#define TC3DATA (TC0DATA + 0x30)
#define TC4DATA (TC0DATA + 0x40)
#define TC5DATA (TC0DATA + 0x50)

#define BSP_WDTCNTRR (TC_BASE + 0x60)
#define BSP_WDTINTRR (BSP_WDTCNTRR + 0x4)
#define BSP_WDTCTRLR (BSP_WDTCNTRR + 0x8)
#	define WDT_E (1<<31) /* Watchdog enable */
#	define WDT_KICK (0x1 << 31) /* Watchdog kick */
#	define WDT_PH12_TO_MSK 31 /* 11111b */
#	define WDT_PH1_TO_SHIFT 22
#	define WDT_PH2_TO_SHIFT 15
#	define WDT_CLK_SC_SHIFT 29

#ifdef CONFIG_CPU_HAS_SYNC
#define __sync_mem()				\
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		"sync 0x3\n\t"			\
		".set	pop"			\
		: /* no output */		\
		: /* no input */		\
		: "memory")
#else
#define __sync_mem()    __sync()
#endif

/*
 * SPI NOR FLASH
 * for drivers/mtd/maps/luna_nor_spi_flash.c
 */
#define SPI_NOR_FLASH_START_ADDR (0x94000000)
#define SPI_NOR_FLASH_MMIO_SIZE  (64*1024*1024)   //64MByte

/*
 * Memory Controller
 */
#define BSP_MC_BASE (0xB8001000)
#define BSP_MC_MCR (BSP_MC_BASE)
#define BSP_MC_MTCR0 (BSP_MC_BASE + 0x04)
#define BSP_MC_MTCR1 (BSP_MC_BASE + 0x08)
#define BSP_MC_PFCR (BSP_MC_BASE + 0x010)
#define BSP_MC_MSRR (BSP_MC_BASE + 0x038)

/* For SRAM Mapping*/
#define R_C0UMSAR0_BASE (0xB8001300)
#define R_C0UMSAR0 (R_C0UMSAR0_BASE + 0x00)
#define R_C0UMSAR1 (R_C0UMSAR0_BASE + 0x10)
#define R_C0UMSAR2 (R_C0UMSAR0_BASE + 0x20)
#define R_C0UMSAR3 (R_C0UMSAR0_BASE + 0x30)

#define R_C0SRAMSAR0_BASE	(0xB8004000)
#define R_C0SRAMSAR0 (R_C0SRAMSAR0_BASE + 0x00)
#define R_C0SRAMSAR1 (R_C0SRAMSAR0_BASE + 0x10)
#define R_C0SRAMSAR2 (R_C0SRAMSAR0_BASE + 0x20)
#define R_C0SRAMSAR3 (R_C0SRAMSAR0_BASE + 0x30)

/*
 * Logical addresses on OCP buses
 * are partitioned into three zones, Zone 0 ~ Zone 2.
 */
/* ZONE0 */
#define  BSP_CDOR0 (0xB8004200)
#define  BSP_CDMAR0 (BSP_CDOR0 + 0x04)
/* ZONE1 */
#define  BSP_CDOR1 (BSP_CDOR0 + 0x10)
#define  BSP_CDMAR1 (BSP_CDOR0 + 0x14)
/* ZONE2 */
#define  BSP_CDOR2 (BSP_CDOR0 + 0x20)
#define  BSP_CDMAR2 (BSP_CDOR0 + 0x24)

/* For PLL query */
#define CG_DEV_CPU0 0x00000000
#define CG_DEV_MEM  0x00000001
#define CG_DEV_LX   0x00000002
#define CG_DEV_SNOF 0x00000003
#define CG_DEV_CPU1 0x00000004
#define CG_DEV_SNAF 0x00000005
uint32_t cg_query_freq(uint32_t dev);
void cg_info_init(void);
extern uint32_t BSP_MHZ;

void prom_printf(char *fmt, ...);
void prom_putchar(char);

void plat_smp_ipi_init(void);
void _rtk_time_init(const int cpu_idx);
void rtk_intc_init(const int cpu_idx);
void rtk_intc_setup(uint32_t irq, uint32_t cpu, uint32_t iv);

#define RTK_CEVT_TIMER_NR 2
static const uint8_t timer_irq[RTK_CEVT_TIMER_NR] = {
	INT_TC0, INT_TC1
};

static const uint32_t timer_base[RTK_CEVT_TIMER_NR] = {
	TC0DATA, TC1DATA
};

#endif /* _BSPCHIP_H */
