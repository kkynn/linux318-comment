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

#include <asm/gic.h>
#include <asm/mips-cm.h>
BUILD_CM_RW(control,		MIPS_CM_GCB_OFS + 0x10)
#define CM_SYNCCTL  (1<<16)

#define PROM_DEBUG      0

/***************************************
 * FPGA 
 * *************************************/
//#define CONFIG_SDK_FPGA_PLATFORM 1
//#define FPGA0202 

#ifdef CONFIG_SDK_FPGA_PLATFORM
	#define MHZ             40 //25
	#define BSP_MHZ			MHZ
	#define BSP_SYSCLK          MHZ * 1000 * 1000
	#if defined(CONFIG_CEVT_R4K)
		/* For R4K on FPGA  */
		#define BSP_CPU0_FREQ           MHZ*1000000     /* 80 MHz */
	#endif
#else
	extern unsigned int BSP_MHZ;
	extern unsigned int BSP_SYSCLK;
#endif

#define BAUDRATE        115200  /* ex. 19200 or 38400 or 57600 or 115200 */                                /* For Early Debug */

/*
 * Register access macro
 */
#ifndef REG32
#define REG32(reg)		(*(volatile unsigned int   *)(reg))
#endif

#define REG16(reg)		(*(volatile unsigned short *)(reg))
#define REG08(reg)		(*(volatile unsigned char  *)(reg))
#define REG8(reg)    (*(volatile unsigned char *)((unsigned int)reg))

#define WRITE_MEM32(addr, val)   (*(volatile unsigned int *)   (addr)) = (val)
#define READ_MEM32(addr)         (*(volatile unsigned int *)   (addr))
#define WRITE_MEM16(addr, val)   (*(volatile unsigned short *) (addr)) = (val)
#define READ_MEM16(addr)         (*(volatile unsigned short *) (addr))
#define WRITE_MEM8(addr, val)    (*(volatile unsigned char *)  (addr)) = (val)
#define READ_MEM8(addr)          (*(volatile unsigned char *)  (addr))

#define SZ_256K			0x00040000


/***************************************************
 *  GIC Interrupt External Source
 * ************************************************/
/* ApolloPro Spec
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

*/
/************************************************************************
* GIC External IRQ Source Num  
* Physical layout
************************************************************************/
#define GIC_EXT_TC0         0
#define GIC_EXT_TC1         1
#define GIC_EXT_TC2         2
#define GIC_EXT_TC3         3
#define GIC_EXT_TC4         4
#define GIC_EXT_TC5         5
#define GIC_EXT_TC6         6
#define GIC_EXT_TC7         7
#define GIC_EXT_TC8         8
#define GIC_EXT_PCM0        9

#define GIC_EXT_PCM1        10
#define GIC_EXT_VOIPXSI     11
#define GIC_EXT_VOIPSPI     12
#define GIC_EXT_VOIPACC     13
#define GIC_EXT_FFTACC      14
#define GIC_EXT_SWCORE      15
#define GIC_EXT_RESERVE16   16
#define GIC_EXT_USBHOSTP2   17
#define GIC_EXT_GPIO_JKMN   18
#define GIC_EXT_RESERVE19   19

#define GIC_EXT_RESERVE20   20
#define GIC_EXT_GPIO_ABCD   21
#define GIC_EXT_GPIO_EFGH   22
#define GIC_EXT_UART0       23
#define GIC_EXT_UART1       24
#define GIC_EXT_UART2       25
#define GIC_EXT_UART3       26
#define GIC_EXT_RESERVE27   27
#define GIC_EXT_RESERVE28   28
#define GIC_EXT_RESERVE29   29

#define GIC_EXT_RESERVE30     30
#define GIC_EXT_RESERVE31     31
#define GIC_EXT_RESERVE32     32
#define GIC_EXT_WDT_PH1TO     33
#define GIC_EXT_WDT_PH2TO     34
#define GIC_EXT_NAND          35
#define GIC_EXT_INT0_GMAC0    36
#define GIC_EXT_INT1_GMAC0    37
#define GIC_EXT_INT0_GMAC1    38
#define GIC_EXT_INT1_GMAC1    39

#define GIC_EXT_INT0_GMAC2      40
#define GIC_EXT_INT1_GMAC2      41
#define GIC_EXT_PONNIC          42
#define GIC_EXT_PONNIC_DS       43
#define GIC_EXT_DBG_LX_BUS_IP   44
#define GIC_EXT_CPI_P1          45
#define GIC_EXT_CPI_P2          46
#define GIC_EXT_CPI_P3          47
#define GIC_EXT_CPI_P4          48
#define GIC_EXT_PCIE_11AC       49

#define GIC_EXT_PCIE_11N           50
#define GIC_EXT_USB_2              51
#define GIC_EXT_USB_23             52
#define GIC_EXT_ECC                53
#define GIC_EXT_DSP_TO_CPU_IE      54
#define GIC_EXT_CPU_TO_DSP_IE      55
/* Reserve 56 ~ 63, Usedd for SMP'S IPI ,4 CPUs */

/************************************************************************
* BSP External IRQ Num  
* ADD IRQ BASE Num
* MIPS_GIC_IRQ_BASE = GIC_IRQ_BASE = 16, defined in <asm/gic.h>
***********************************************************************/
#define BSP_TC0_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC0           )
#define BSP_TC1_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC1           )
#define BSP_TC2_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC2           )
#define BSP_TC3_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC3           )
#define BSP_TC4_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC4           )
#define BSP_TC5_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC5           )
#define BSP_TC6_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC6           )
#define BSP_TC7_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC7           )
#define BSP_TC8_IRQ         ( MIPS_GIC_IRQ_BASE +  GIC_EXT_TC8           )
#define BSP_PCM0_IRQ        ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PCM0          )
#define BSP_PCM1_IRQ        ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PCM1          )
#define BSP_VOIPXSI_IRQ     ( MIPS_GIC_IRQ_BASE +  GIC_EXT_VOIPXSI       )
#define BSP_VOIPSPI_IRQ     ( MIPS_GIC_IRQ_BASE +  GIC_EXT_VOIPSPI       )
#define BSP_VOIPACC_IRQ     ( MIPS_GIC_IRQ_BASE +  GIC_EXT_VOIPACC       )
#define BSP_FFTACC_IRQ      ( MIPS_GIC_IRQ_BASE +  GIC_EXT_FFTACC        )
#define BSP_SWCORE_IRQ      ( MIPS_GIC_IRQ_BASE +  GIC_EXT_SWCORE        )
#define BSP_RESERVE16_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE16     )
#define BSP_USBHOSTP2_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_USBHOSTP2     )
#define BSP_GPIO_JKMN_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_GPIO_JKMN     )
#define BSP_RESERVE19_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE19     )
#define BSP_RESERVE20_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE20     )
#define BSP_GPIO_ABCD_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_GPIO_ABCD     )
#define BSP_GPIO_EFGH_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_GPIO_EFGH     )
#define BSP_UART0_IRQ       ( MIPS_GIC_IRQ_BASE +  GIC_EXT_UART0         )
#define BSP_UART1_IRQ       ( MIPS_GIC_IRQ_BASE +  GIC_EXT_UART1         )
#define BSP_UART2_IRQ       ( MIPS_GIC_IRQ_BASE +  GIC_EXT_UART2         )
#define BSP_UART3_IRQ       ( MIPS_GIC_IRQ_BASE +  GIC_EXT_UART3         )
#define BSP_RESERVE27_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE27     )
#define BSP_RESERVE28_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE28     )
#define BSP_RESERVE29_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE29     )
#define BSP_RESERVE30_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE30       )
#define BSP_RESERVE31_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE31       )
#define BSP_RESERVE32_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_RESERVE32       )
#define BSP_WDT_PH1TO_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_WDT_PH1TO       )
#define BSP_WDT_PH2TO_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_WDT_PH2TO       )
#define BSP_NAND_IRQ        ( MIPS_GIC_IRQ_BASE +  GIC_EXT_NAND            )
#define BSP_INT0_GMAC0_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT0_GMAC0      )
#define BSP_INT1_GMAC0_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT1_GMAC0      )
#define BSP_INT0_GMAC1_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT0_GMAC1      )
#define BSP_INT1_GMAC1_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT1_GMAC1      )
#define BSP_INT0_GMAC2_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT0_GMAC2       )
#define BSP_INT1_GMAC2_IRQ  ( MIPS_GIC_IRQ_BASE +  GIC_EXT_INT1_GMAC2       )
#define BSP_PONNIC_IRQ      ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PONNIC           )
#define BSP_PONNIC_DS_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PONNIC_DS        )
#define BSP_DBG_LX_BUS_IP_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_DBG_LX_BUS_IP    )
#define BSP_CPI_P1_IRQ          ( MIPS_GIC_IRQ_BASE +  GIC_EXT_CPI_P1           )
#define BSP_CPI_P2_IRQ          ( MIPS_GIC_IRQ_BASE +  GIC_EXT_CPI_P2           )
#define BSP_CPI_P3_IRQ          ( MIPS_GIC_IRQ_BASE +  GIC_EXT_CPI_P3           )
#define BSP_CPI_P4_IRQ          ( MIPS_GIC_IRQ_BASE +  GIC_EXT_CPI_P4           )
#define BSP_PCIE_11AC_IRQ       ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PCIE_11AC        )
#define BSP_PCIE_11N_IRQ        ( MIPS_GIC_IRQ_BASE +  GIC_EXT_PCIE_11N         )
#define BSP_USB_2_IRQ           ( MIPS_GIC_IRQ_BASE +  GIC_EXT_USB_2            )
#define BSP_USB_23_IRQ          ( MIPS_GIC_IRQ_BASE +  GIC_EXT_USB_23           )
#define BSP_ECC_IRQ             ( MIPS_GIC_IRQ_BASE +  GIC_EXT_ECC              )
#define BSP_DSP_TO_CPU_IE_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_DSP_TO_CPU_IE    )
#define BSP_CPU_TO_DSP_IE_IRQ   ( MIPS_GIC_IRQ_BASE +  GIC_EXT_CPU_TO_DSP_IE    )


#if 0
/*
 *  UART 
 */
#define BSP_UART0_BASE		0xb8002000UL
#define BSP_UART0_BAUD		38400  /* ex. 19200 or 38400 or 57600 or 115200 */  
#define BSP_UART0_FREQ		(SYSCLK - BSP_UART0_BAUD * 24)
#define BSP_UART0_PADDR   	0x18002000UL
#define BSP_UART0_PSIZE         0x100

#define BSP_UART1_BASE		0xb8002100UL
#define BSP_UART1_BAUD		38400  /* ex. 19200 or 38400 or 57600 or 115200 */  
#define BSP_UART1_FREQ		(SYSCLK - BSP_UART0_BAUD * 24)
#define BSP_UART1_MAPBASE	0x18002100UL
#define BSP_UART1_MAPSIZE	0x100

#define UART0_RBR		(BSP_UART0_BASE + 0x000)
#define UART0_THR		(BSP_UART0_BASE + 0x000)
#define UART0_DLL		(BSP_UART0_BASE + 0x000)
#define UART0_IER		(BSP_UART0_BASE + 0x004)
#define UART0_DLM		(BSP_UART0_BASE + 0x004)
#define UART0_IIR		(BSP_UART0_BASE + 0x008)
#define UART0_FCR		(BSP_UART0_BASE + 0x008)
#define UART0_LCR		(BSP_UART0_BASE + 0x00C)
#define UART0_MCR		(BSP_UART0_BASE + 0x010)
#define UART0_LSR		(BSP_UART0_BASE + 0x014)

#define UART1_RBR		(BSP_UART1_BASE + 0x000)
#define UART1_THR		(BSP_UART1_BASE + 0x000)
#define UART1_DLL		(BSP_UART1_BASE + 0x000)
#define UART1_IER		(BSP_UART1_BASE + 0x004)
#define UART1_DLM		(BSP_UART1_BASE + 0x004)
#define UART1_IIR		(BSP_UART1_BASE + 0x008)
#define UART1_FCR		(BSP_UART1_BASE + 0x008)
   #define FCR_EN		0x01
   #define FCR_RXRST		0x02
   #define     RXRST		0x02
   #define FCR_TXRST		0x04
   #define     TXRST		0x04
   #define FCR_DMA		0x08
   #define FCR_RTRG		0xC0
   #define     CHAR_TRIGGER_01	0x00
   #define     CHAR_TRIGGER_04	0x40
   #define     CHAR_TRIGGER_08	0x80
   #define     CHAR_TRIGGER_14	0xC0
#define UART1_LCR		(BSP_UART1_BASE + 0x00C)
   #define LCR_WLN		0x03
   #define     CHAR_LEN_5	0x00
   #define     CHAR_LEN_6	0x01
   #define     CHAR_LEN_7	0x02
   #define     CHAR_LEN_8	0x03
   #define LCR_STB		0x04
   #define     ONE_STOP		0x00
   #define     TWO_STOP		0x04
   #define LCR_PEN		0x08
   #define     PARITY_ENABLE	0x01
   #define     PARITY_DISABLE	0x00
   #define LCR_EPS		0x30
   #define     PARITY_ODD	0x00
   #define     PARITY_EVEN	0x10
   #define     PARITY_MARK	0x20
   #define     PARITY_SPACE	0x30
   #define LCR_BRK		0x40
   #define LCR_DLAB		0x80
   #define     DLAB		0x80
#define UART1_MCR		(BSP_UART1_BASE + 0x010)
#define UART1_LSR		(BSP_UART1_BASE + 0x014)
   #define LSR_DR		0x01
   #define     RxCHAR_AVAIL	0x01
   #define LSR_OE		0x02
   #define LSR_PE		0x04
   #define LSR_FE		0x08
   #define LSR_BI		0x10
   #define LSR_THRE		0x20
   #define     TxCHAR_AVAIL	0x00
   #define     TxCHAR_EMPTY	0x20
   #define LSR_TEMT		0x40
   #define LSR_RFE		0x80

#endif

/*
 * Timer/Counter/Watchdog for ApolloPro chip
 */
#define TC_MAX          8
#define TC_PBASE        0x18003200    //Physical address
#define TC_MAPSIZE      0xA0         //Physical range for 8 Timer and watchdog used by CPU

#define TC_BASE         0xB8003200
#define TC0DATA         (TC_BASE + 0x00)

#define APOLLOPRO_TC0DATA          (TC_BASE)
#define APOLLOPRO_TC1DATA          (TC_BASE + 0x10)
#define APOLLOPRO_TC2DATA          (TC_BASE + 0x20)
#define APOLLOPRO_TC3DATA          (TC_BASE + 0x30)
#define APOLLOPRO_TC4DATA          (TC_BASE + 0x40)
#define APOLLOPRO_TC5DATA          (TC_BASE + 0x50)

#define APOLLOPRO_TC0CNT           (TC_BASE + 0x04)
#define APOLLOPRO_TC1CNT           (TC_BASE + 0x14)
#define APOLLOPRO_TC2CNT           (TC_BASE + 0x24)
#define APOLLOPRO_TC3CNT           (TC_BASE + 0x34)
#define APOLLOPRO_TC4CNT           (TC_BASE + 0x44)
#define APOLLOPRO_TC5CNT           (TC_BASE + 0x54)

#define APOLLOPRO_TC0CTL           (TC_BASE + 0x08)
#define APOLLOPRO_TC1CTL           (TC_BASE + 0x18)
#define APOLLOPRO_TC2CTL           (TC_BASE + 0x28)
#define APOLLOPRO_TC3CTL           (TC_BASE + 0x38)
#define APOLLOPRO_TC4CTL           (TC_BASE + 0x48)
#define APOLLOPRO_TC5CTL           (TC_BASE + 0x58)
   #define APOLLOPRO_TCEN          (1 << 28)
   #define APOLLOPRO_TCMODE_TIMER  (1 << 24)
   #define APOLLOPRO_TCDIV_FACTOR  (0xFFFF << 0)

#define APOLLOPRO_TC0INT           (TC_BASE + 0xC)
#define APOLLOPRO_TC1INT           (TC_BASE + 0x1C)
#define APOLLOPRO_TC2INT           (TC_BASE + 0x2C)
#define APOLLOPRO_TC3INT           (TC_BASE + 0x3C)
#define APOLLOPRO_TC4INT           (TC_BASE + 0x4C)
#define APOLLOPRO_TC5INT           (TC_BASE + 0x5C)
   #define APOLLOPRO_TCIE          (1 << 20)
   #define APOLLOPRO_TCIP          (1 << 16)
/* 	Luna Watchdog  */
#define BSP_WDTCNTRR          (TC_BASE + 0x60)
#define BSP_WDTINTRR          (BSP_WDTCNTRR + 0x4)
#define BSP_WDTCTRLR          (BSP_WDTCNTRR + 0x8)
	#define WDT_E                         (1<<31)           /* Watchdog enable */
	#define WDT_KICK                      (0x1 << 31)       /* Watchdog kick */
	#define WDT_PH12_TO_MSK               31               /* 11111b */
	#define WDT_PH1_TO_SHIFT              22
	#define WDT_PH2_TO_SHIFT              15
	#define WDT_CLK_SC_SHIFT              29



#define DIVISOR_APOLLOPRO        1000
#if DIVISOR_APOLLOPRO > (1 << 16)
#error "Exceed the Maximum Value of DivFactor"
#endif

/**************************************************
 * For L2 Cache initialization 
***************************************************/
#define APOLLOPRO_L2CONFIG     0xB8000214
#define L2_DEBUG
#if defined(L2_DEBUG)
#define PRINTK_L2(fmt, ...)   printk("[L2_DEBUG:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_L2(fmt, ...)
#endif

#define L2_BYPASS_MODE                      (1<<12)
/*************************************************
 * Multi-Cycle Data Ram Operation 
 * 00 1 cycle access, no stalling 
 * 01 2 cycle access, 1 stall 
 * 10 3 cycle access, 2 stalls 
 * 11 4 cycle access, 3 stalls
**************************************************/
#define APOLLOPRO_L2_DATASTALL_VALUE        0x0

#define APOLLOPRO_L2_DATASTALL_SHT          0x4
#define APOLLOPRO_L2_DATASTALL_MASK         0x3


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
//#define SPI_NOR_FLASH_START_ADDR (0xBD000000)
//#define SPI_NOR_FLASH_MMIO_SIZE  (32*1024*1024) //32MByte
#define SPI_NOR_FLASH_START_ADDR (0xB4000000)
#define SPI_NOR_FLASH_MMIO_SIZE  (64*1024*1024)   //64MByte


/*
 * Memory Controller
 */
#define BSP_MC_BASE         0xB8001000
#define BSP_MC_MCR          (BSP_MC_BASE)
#define BSP_MC_MTCR0        (BSP_MC_BASE + 0x04)
#define BSP_MC_MTCR1        (BSP_MC_BASE + 0x08)
#define BSP_MC_PFCR         (BSP_MC_BASE + 0x010)
#define BSP_MC_MSRR    (BSP_MC_BASE + 0x038)
#define FLUSH_OCP_CMD    (1<<31)
#define BSP_NCR             (BSP_MC_BASE + 0x100)
#define BSP_NSR             (BSP_MC_BASE + 0x104)
#define BSP_NCAR            (BSP_MC_BASE + 0x108)
#define BSP_NADDR           (BSP_MC_BASE + 0x10C)
#define BSP_NDR             (BSP_MC_BASE + 0x110)

#define BSP_SFCR            (BSP_MC_BASE + 0x200)
#define BSP_SFDR            (BSP_MC_BASE + 0x204)

#define BSP_BOOT_FLASH_STS		(0xf << 24)

/* For SRAM Mapping*/
#define BSP_SOC_SRAM_SIZE  (32*1024) //32KByte
#define BSP_SOC_SRAM_BASE  (192*1024) //0x30000,32MByte
#define DRAM_SIZE_32KB      	(0x8)

/*
*	SRAM DRAM control registers
*/
//	CPU0
//	Unmaped Memory Segment Address Register
#define R_C0UMSAR0_BASE		(0xB8001300) /* DRAM UNMAP */
//#ifdef CONFIG_LUNA_USE_SRAM
#define R_C0UMSAR0 			(R_C0UMSAR0_BASE + 0x00)
#define R_C0UMSAR1 			(R_C0UMSAR0_BASE + 0x10)
#define R_C0UMSAR2 			(R_C0UMSAR0_BASE + 0x20)
#define R_C0UMSAR3 			(R_C0UMSAR0_BASE + 0x30)

#define R_C0SRAMSAR0_BASE	(0xB8004000)
#define R_C0SRAMSAR0		(R_C0SRAMSAR0_BASE + 0x00)
#define R_C0SRAMSAR1		(R_C0SRAMSAR0_BASE + 0x10)
#define R_C0SRAMSAR2		(R_C0SRAMSAR0_BASE + 0x20)
#define R_C0SRAMSAR3		(R_C0SRAMSAR0_BASE + 0x30)
//#endif

/* For DRAM controller */
#define C0DCR		(0xB8001004)
/* 
 * Logical addresses on OCP buses 
 * are partitioned into three zones, Zone 0 ~ Zone 2.
 */
/* ZONE0 */
#define  BSP_CDOR0         0xB8004200
#define  BSP_CDMAR0       (BSP_CDOR0 + 0x04)
/* ZONE1 */
#define  BSP_CDOR1         (BSP_CDOR0 + 0x10)
#define  BSP_CDMAR1       (BSP_CDOR0 + 0x14)
/* ZONE2 */
#define  BSP_CDOR2         (BSP_CDOR0 + 0x20)
#define  BSP_CDMAR2       (BSP_CDOR0 + 0x24)

#if defined(CONFIG_RTL9607C)
#include "bspchip_9607c.h"
#else
error
#endif

#endif   /* _BSPCHIP_H */
