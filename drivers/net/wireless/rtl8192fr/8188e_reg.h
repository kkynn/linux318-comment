/*
 *  Header file of 8188E register
 *
 *  Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef _8188E_REG_H_
#define _8188E_REG_H_

#ifndef WLAN_HAL_INTERNAL_USED


//============================================================
//       8188E Regsiter offset definition
//============================================================


/*
 *	Project RTL8188E follows most of registers in Project RTL8192c
 *	This file includes additional registers for RTL8188E only
 *	Header file of RTL8192C registers should always be included
 */

//
// 1. System Configure Register. (Offset 0x000 - 0x0FFh)
//
#define		REG_88E_BB_PAD_CTRL			0x64
#define		REG_88E_HMEBOX_E0			0x88
#define		REG_88E_HMEBOX_E1			0x8A
#define		REG_88E_HMEBOX_E2			0x8C
#define		REG_88E_HMEBOX_E3			0x8E
#define		REG_88E_WLLPS_CTRL			0x90
#define		REG_88E_RPWM2					0x9E
#define		REG_88E_HIMR					0xB0
#define		REG_88E_HISR					0xB4
#define		REG_88E_HIMRE					0xB8
#define		REG_88E_HISRE					0xBC
#define		REG_88E_EFUSE_DATA1			0xCC
#define		REG_88E_EFUSE_DATA0			0xCD
#define		REG_88E_EPPR					0xCF
#define		REG_88E_TXDMA_TH				0x218
#define		REG_88E_LQ_TH					0x21C

#define		REG_88E_WATCHDOG				0x368

#define		REG_88E_MACID_NOLINK			0x484
#define		REG_88E_MACID_PAUSE			0x48C
#define		REG_88E_TXRPT_CTRL			0x4EC
#define		REG_88E_TXRPT_TIM				0x4F0
#define		REG_88E_TXRPT_STSSET			0x4F2
#define		REG_88E_TXRPT_STSVLD			0x4F4
#define		REG_88E_TXRPT_STSINF			0x4F8



//============================================================
//       Registers for 8188E IQK
//============================================================


#define		REG_FPGA0_IQK					0xe28
#define		REG_TX_IQK_TONE_A				0xe30
#define		REG_RX_IQK_TONE_A				0xe34
#define		REG_TX_IQK_PI_A				0xe38
#define		REG_RX_IQK_PI_A				0xe3c

#define		REG_TX_IQK 					0xe40
#define		REG_RX_IQK						0xe44
#define		REG_IQK_AGC_PTS				0xe48
#define		REG_IQK_AGC_RSP				0xe4c
#define		REG_TX_IQK_TONE_B				0xe50
#define		REG_RX_IQK_TONE_B				0xe54
#define		REG_TX_IQK_PI_B				0xe58
#define		REG_RX_IQK_PI_B				0xe5c
#define		REG_IQK_AGC_CONT				0xe60

#define		rRx_Power_Before_IQK_A		0xea0
#define		REG_RX_POWER_BEFORE_IQK_A_2	0xea4
#define		rRx_Power_After_IQK_A		0xea8
#define		REG_RX_POWER_AFTER_IQK_A_2		0xeac

#define		REG_TX_POWER_BEFORE_IQK_A		0xe94
#define		REG_TX_POWER_AFTER_IQK_A		0xe9c


#define		REG_TX_POWER_BEFORE_IQK_B		0xeb4
#define		REG_TX_POWER_AFTER_IQK_B		0xebc

#define		rRx_Power_Before_IQK_B		0xec0
#define		REG_RX_POWER_BEFORE_IQK_B_2	0xec4
#define		rRx_Power_After_IQK_B		0xec8
#define		REG_RX_POWER_AFTER_IQK_B_2		0xecc


#define		RF_RCK_OS					0x30	// RF TX PA control
#define		RF_TXPA_G1					0x31	// RF TX PA control
#define		RF_TXPA_G2					0x32	// RF TX PA control
#define		RF_TXPA_G3					0x33	// RF TX PA control
#define		RF_TX_BIAS_A				0x35
#define		RF_TX_BIAS_D				0x36
#define		RF_LOBF_9					0x38
#define		RF_RXRF_A3					0x3C	//	
#define		RF_TRSW						0x3F

#define		RF_TXRF_A2					0x41
#define		RF_TXPA_G4					0x46	
#define		RF_TXPA_A4					0x4B	

#define		RF_WE_LUT					0xEF


//#define		rFPGA0_XAB_SwitchControl	0x858	// RF Channel switch
//#define		REG_FPGA0_XCD_SWITCH_CONTROL	0x85c

//#define		REG_FPGA0_XAB_RF_INTERFACE_SW		0x870	// RF Interface Software Control
//#define		REG_FPGA0_XCD_RF_INTERFACE_SW		0x874

//#define		REG_FPGA0_XAB_RF_PARAMETER		0x878	// RF Parameter
//#define		rFPGA0_XCD_RFParameter		0x87c

//#define		rFPGA0_AnalogParameter1		0x880	// Crystal cap setting RF-R/W protection for parameter4??
//#define		rFPGA0_AnalogParameter2		0x884
//#define		rFPGA0_AnalogParameter3		0x888
//#define		rFPGA0_AdDaClockEn			0x888	// enable ad/da clock1 for dual-phy
//#define		REG_FPGA0_ANALOG_PARAMETER4		0x88c

//#define		rFPGA0_XA_LSSIReadBack		0x8a0	// Tranceiver LSSI Readback
//#define		REG_FPGA0_XB_LSSI_READ_BACK		0x8a4
#define		rFPGA0_XC_LSSIReadBack		0x8a8
#define		rFPGA0_XD_LSSIReadBack		0x8ac

//#define		REG_FPGA0_PSD_REPORT				0x8b4	// Useless now
//#define		TransceiverA_HSPI_Readback		0x8b8	// Transceiver A HSPI Readback
//#define		TransceiverB_HSPI_Readback		0x8bc	// Transceiver B HSPI Readback
//#define		rFPGA0_XAB_RFInterfaceRB		0x8e0	// Useless now // RF Interface Readback Value
#define		rFPGA0_XCD_RFInterfaceRB		0x8e4	// Useless now


#define		REG_BLUE_TOOTH					0xe6c
#define		REG_RX_WAIT_CCA				0xe70
#define		REG_TX_CCK_RFON				0xe74
#define		REG_TX_CCK_BBON				0xe78
#define		REG_TX_OFDM_RFON				0xe7c
#define		REG_TX_OFDM_BBON				0xe80
#define		REG_TX_TO_RX					0xe84
#define		REG_TX_TO_TX					0xe88
#define		REG_RX_CCK						0xe8c

#define		REG_RX_OFDM					0xed0
#define		REG_RX_WAIT_RIFS 				0xed4
#define		REG_RX_TO_RX 					0xed8
#define		REG_STANDBY 					0xedc
#define		REG_SLEEP 						0xee0
#define		REG_PMPD_ANAEN					0xeec


#define REG_EDCA_VO_PARAM			0x0500
#define REG_EDCA_VI_PARAM			0x0504
#define REG_EDCA_BE_PARAM			0x0508
#define REG_EDCA_BK_PARAM			0x050C
#define REG_BCNTCFG					0x0510
#define REG_PIFS					0x0512
#define REG_RDG_PIFS				0x0513
#define REG_SIFS_CTX				0x0514
#define REG_SIFS_TRX				0x0516
#define REG_TSFTR_SNC_OFFSET		0x0518
#define REG_AGGR_BREAK_TIME			0x051A
#define REG_SLOT					0x051B
#define REG_TX_PTCL_CTRL			0x0520
#define REG_TXPAUSE					0x0522
#define REG_DIS_TXREQ_CLR			0x0523
#define REG_RD_CTRL					0x0524


#define REG_TBTT_PROHIBIT			0x0540
#define REG_RD_NAV_NXT				0x0544
#define REG_NAV_PROT_LEN			0x0546
#define REG_BCN_CTRL				0x0550
#define REG_BCN_CTRL_1				0x0551
#define REG_MBID_NUM				0x0552
#define REG_DUAL_TSF_RST			0x0553
#define REG_BCN_INTERVAL			0x0554	// The same as REG_MBSSID_BCN_SPACE
#define REG_DRVERLYINT				0x0558
#define REG_BCNDMATIM				0x0559
#define REG_ATIMWND					0x055A
#define REG_BCN_MAX_ERR				0x055D
#define REG_RXTSF_OFFSET_CCK		0x055E
#define REG_RXTSF_OFFSET_OFDM		0x055F	
#define REG_TSFTR					0x0560
#define REG_TSFTR1					0x0568				// HW Port 1 TSF Register
#define REG_P2P_CTWIN				0x0572 // 1 Byte long (in unit of TU)
#define REG_PSTIMER					0x0580
#define REG_TIMER0					0x0584
#define REG_TIMER1					0x0588
#define REG_ACMHWCTRL				0x05C0
#define REG_NOA_DESC_SEL			0x05CF
#define REG_NOA_DESC_DURATION		0x05E0
#define REG_NOA_DESC_INTERVAL		0x05E4
#define REG_NOA_DESC_START			0x05E8
#define REG_NOA_DESC_COUNT			0x05EC


#define REG_SYS_ISO_CTRL				0x0000
//#define REG_SYS_FUNC_EN					0x0002
#define REG_APS_FSMCO					0x0004
#define REG_SYS_CLKR					0x0008
#define REG_9346CR						0x000A
#define REG_EE_VPD						0x000C
#define REG_AFE_MISC					0x0010
#define REG_SPS0_CTRL					0x0011
#define REG_SPS0_CTRL_6					0x0016
#define REG_POWER_OFF_IN_PROCESS 		0x0017
#define REG_SPS_OCP_CFG					0x0018
#define REG_RSV_CTRL					0x001C
//#define REG_RF_CTRL						0x001F
#define REG_LDOA15_CTRL					0x0020
#define REG_LDOV12D_CTRL				0x0021
//#define REG_LDOHCI12_CTRL				0x0022
#define REG_LPLDO_CTRL					0x0023
//#define REG_AFE_XTAL_CTRL				0x0024
//#define REG_AFE_PLL_CTRL				0x0028
#define REG_MAC_PHY_CTRL				0x002c //for 92d, DMDP,SMSP,DMSP contrl
#define REG_EFUSE_CTRL					0x0030
#define REG_EFUSE_TEST					0x0034
#define REG_PWR_DATA					0x0038
#define REG_CAL_TIMER					0x003C
#define REG_ACLK_MON					0x003E
#define REG_GPIO_MUXCFG					0x0040
#define REG_GPIO_IO_SEL					0x0042
#define REG_MAC_PINMUX_CFG				0x0043
#define REG_GPIO_PIN_CTRL				0x0044
#define REG_GPIO_INTM					0x0048
#define REG_LEDCFG0						0x004C
#define REG_LEDCFG1						0x004D
#define REG_LEDCFG2						0x004E
#define REG_LEDCFG3						0x004F
#define REG_FSIMR						0x0050
#define REG_FSISR						0x0054
#define REG_HSIMR						0x0058
#if defined(REG_HSISR)	
	#undef  REG_HSISR   /*redefined*/
#endif	
#define REG_HSISR						0x005c
#define REG_GPIO_PIN_CTRL_2				0x0060 // RTL8723 WIFI/BT/GPS Multi-Function GPIO Pin Control.
#define REG_GPIO_IO_SEL_2				0x0062 // RTL8723 WIFI/BT/GPS Multi-Function GPIO Select.
#define REG_MULTI_FUNC_CTRL				0x0068 // RTL8723 WIFI/BT/GPS Multi-Function control source.
#define REG_GPIO_OUTPUT					0x006c
#define REG_AFE_XTAL_CTRL_EXT			0x0078 //RTL8188E
#define REG_XCK_OUT_CTRL				0x007c //RTL8188E
//#define REG_MCUFWDL						0x0080
#define	REG_WOL_EVENT					0x0081 //RTL8188E
#define REG_MCUTSTCFG					0x0084
#define REG_HMEBOX_EXT_0				0x0088
#define REG_HMEBOX_EXT_1				0x008A
#define REG_HMEBOX_EXT_2				0x008C
#define REG_HMEBOX_EXT_3				0x008E
#define REG_HOST_SUSP_CNT				0x00BC	// RTL8192C Host suspend counter on FPGA platform
#define REG_HIMR_88E					0x00B0 //RTL8188E
#define REG_HISR_88E					0x00B4 //RTL8188E
#define REG_HIMRE_88E					0x00B8 //RTL8188E
#define REG_HISRE_88E					0x00BC //RTL8188E
#define REG_EFUSE_ACCESS				0x00CF	// Efuse access protection for RTL8723
#define REG_BIST_SCAN					0x00D0
#define REG_BIST_RPT					0x00D4
#define REG_BIST_ROM_RPT				0x00D8
#define REG_USB_SIE_INTF				0x00E0
#define REG_PCIE_MIO_INTF				0x00E4
#define REG_PCIE_MIO_INTD				0x00E8
#define REG_HPON_FSM					0x00EC
#define REG_SYS_CFG						0x00F0
#define REG_GPIO_OUTSTS					0x00F4	// For RTL8723 only.
#define REG_TYPE_ID						0x00FC	

#define REG_32K_CTRL					0x0194 //RTL8188E

#define		REG_CONFIG_ANT_A 				0xb68
#define		REG_CONFIG_ANT_B 				0xb6c


//============================================================================
//       8188E Regsiter Bit and Content definition
//============================================================================


//----------------------------------------------------------------------------
//       8188E REG_88E_BB_PAD_CTRL bits			(Offset 0x64-66, 24 bits)
//----------------------------------------------------------------------------
#define	BB_PAD_CTRL_88E_PAPE_EN			BIT(19)		// PAD ��P_LNAON�� output enable
#define	BB_PAD_CTRL_88E_PAPE_DRV			BIT(18)		// PAD ��P_LNAON�� output value
#define	BB_PAD_CTRL_88E_LNAON_SR		BIT(17)		// Control SR of PAD ��P_LNAON�� to control the slew rate
#define	BB_PAD_CTRL_88E_LNAON_E2		BIT(16)		// Control E2 of PAD ��P_LNAON�� for its output driving capability
#define	BB_PAD_CTRL_88E_TRSW_EN			BIT(11)		// PAD ��P_TRSWP�� and ��P_TRSWN�� output enable
#define	BB_PAD_CTRL_88E_TRSW_DRV		BIT(10)		// PADs ��P_TRSWP�� and ��P_TRSWN�� outputs values


//----------------------------------------------------------------------------
//       8188E REG_88E_WLLPS_CTRL bits		(Offset 0x90-93, 32 bits)
//----------------------------------------------------------------------------
#define	WLLPS_CTRL_88E_EABM			BIT(31)
#define	WLLPS_CTRL_88E_ACKF			BIT(30)
#define	WLLPS_CTRL_88E_ESWR			BIT(28)
#define	WLLPS_CTRL_88E_PWMM			BIT(27)
#define	WLLPS_CTRL_88E_EECK			BIT(26)
#define	WLLPS_CTRL_88E_ELDO			BIT(25)
#define	WLLPS_CTRL_88E_EXTAL			BIT(24)
#define	WLLPS_CTRL_88E_LPS_EN		BIT(0)


//----------------------------------------------------------------------------
//       8188E REG_88E_HIMR bits				(Offset 0xB0-B3, 32 bits)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//       8188E REG_88E_HISR bits				(Offset 0xB4-B7, 32 bits)
//----------------------------------------------------------------------------
#define	HIMR_88E_TXCCK				BIT(30)		// TXRPT interrupt when CCX bit of the packet is set	
#define	HIMR_88E_PSTIMEOUT			BIT(29)		// Power Save Time Out Interrupt
#define	HIMR_88E_GTINT4				BIT(28)		// When GTIMER4 expires, this bit is set to 1	
#define	HIMR_88E_GTINT3				BIT(27)		// When GTIMER3 expires, this bit is set to 1	
#define	HIMR_88E_TBDER				BIT(26)		// Transmit Beacon0 Error			
#define	HIMR_88E_TBDOK				BIT(25)		// Transmit Beacon0 OK, ad hoc only
#define	HIMR_88E_TSF_BIT32_TOGGLE	BIT(24)		// TSF Timer BIT32 toggle indication interrupt			
#define	HIMR_88E_BcnInt				BIT(20)		// Beacon DMA Interrupt 0			
#define	HIMR_88E_BDOK					BIT(16)		// Beacon Queue DMA OK0			
#define	HIMR_88E_HSISR_IND_ON_INT	BIT(15)		// HSISR Indicator (HSIMR & HSISR is true, this bit is set to 1)			
#define	HIMR_88E_BCNDMAINT_E			BIT(14)		// Beacon DMA Interrupt Extension for Win7			
#define	HIMR_88E_ATIMEND				BIT(12)		// CTWidnow End or ATIM Window End
#define	HIMR_88E_HISR1_IND_INT		BIT(11)		// HISR1 Indicator (HISR1 & HIMR1 is true, this bit is set to 1)
#define	HIMR_88E_C2HCMD				BIT(10)		// CPU to Host Command INT Status, Write 1 clear	
#define	HIMR_88E_CPWM2				BIT(9)		// CPU power Mode exchange INT Status, Write 1 clear	
#define	HIMR_88E_CPWM					BIT(8)		// CPU power Mode exchange INT Status, Write 1 clear	
#define	HIMR_88E_HIGHDOK				BIT(7)		// High Queue DMA OK	
#define	HIMR_88E_MGNTDOK				BIT(6)		// Management Queue DMA OK	
#define	HIMR_88E_BKDOK				BIT(5)		// AC_BK DMA OK		
#define	HIMR_88E_BEDOK				BIT(4)		// AC_BE DMA OK	
#define	HIMR_88E_VIDOK				BIT(3)		// AC_VI DMA OK		
#define	HIMR_88E_VODOK				BIT(2)		// AC_VO DMA OK	
#define	HIMR_88E_RDU					BIT(1)		// Rx Descriptor Unavailable	
#define	HIMR_88E_ROK					BIT(0)		// Receive DMA OK


//----------------------------------------------------------------------------
//       8188E REG_88E_HIMRE bits			(Offset 0xB8-BB, 32 bits)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//       8188E REG_88E_HIMSE bits			(Offset 0xBC-BF, 32 bits)
//----------------------------------------------------------------------------
#define	HIMRE_88E_BCNDMAINT7			BIT(27)		// Beacon DMA Interrupt 7
#define	HIMRE_88E_BCNDMAINT6			BIT(26)		// Beacon DMA Interrupt 6
#define	HIMRE_88E_BCNDMAINT5			BIT(25)		// Beacon DMA Interrupt 5
#define	HIMRE_88E_BCNDMAINT4			BIT(24)		// Beacon DMA Interrupt 4
#define	HIMRE_88E_BCNDMAINT3			BIT(23)		// Beacon DMA Interrupt 3
#define	HIMRE_88E_BCNDMAINT2			BIT(22)		// Beacon DMA Interrupt 2
#define	HIMRE_88E_BCNDMAINT1			BIT(21)		// Beacon DMA Interrupt 1
#define	HIMRE_88E_BCNDOK7			BIT(20)		// Beacon Queue DMA OK Interrup 7
#define	HIMRE_88E_BCNDOK6			BIT(19)		// Beacon Queue DMA OK Interrup 6
#define	HIMRE_88E_BCNDOK5			BIT(18)		// Beacon Queue DMA OK Interrup 5
#define	HIMRE_88E_BCNDOK4			BIT(17)		// Beacon Queue DMA OK Interrup 4
#define	HIMRE_88E_BCNDOK3			BIT(16)		// Beacon Queue DMA OK Interrup 3
#define	HIMRE_88E_BCNDOK2			BIT(15)		// Beacon Queue DMA OK Interrup 2
#define	HIMRE_88E_BCNDOK1			BIT(14)		// Beacon Queue DMA OK Interrup 1
#define	HIMRE_88E_ATIMEND_E			BIT(13)		// ATIM Window End Extension for Win7
#define	HIMRE_88E_TXERR				BIT(11)		// Tx Error Flag Interrupt Status, write 1 clear.
#define	HIMRE_88E_RXERR				BIT(10)		// Rx Error Flag INT Status, Write 1 clear
#define	HIMRE_88E_TXFOVW				BIT(9)		// Transmit FIFO Overflow
#define	HIMRE_88E_RXFOVW				BIT(8)		// Receive FIFO Overflow


//----------------------------------------------------------------------------
//       8188E REG_EFUSE_ACCESS			(Offset 0xCF, 8 bits)
//----------------------------------------------------------------------------
#define EFUSE_ACCESS_ON			0x69	// For RTL8723 only.
#define EFUSE_ACCESS_OFF			0x00	// For RTL8723 only.


//----------------------------------------------------------------------------
//       8188E REG_88E_WATCHDOG bits				(Offset 0x368-369, 16 bits)
//----------------------------------------------------------------------------
#define	WATCHDOG_88E_ENABLE					BIT(15)		// Enable lbc timeout watchdog
#define	WATCHDOG_88E_R_IO_TIMEOUT_FLAG		BIT(14)		// Lbc timeout flag.Write ��1�� to clear
#define	WATCHDOG_88E_RECORD_Mask			0x3FFF		// Time out register address


//----------------------------------------------------------------------------
//       8188E REG_88E_TXRPT_CTRL bits				(Offset 0x4EC-4EF, 32 bits)
//----------------------------------------------------------------------------
#define	TXRPT_CTRL_88E_CNT_TH_SHIFT			16
#define	TXRPT_CTRL_88E_CNT_TH_Mask			0xFFFF
#define	TXRPT_CTRL_88E_RPT_MACID_SHIFT		8
#define	TXRPT_CTRL_88E_RPT_MACID_Mask		0x7F
#define	TXRPT_CTRL_88E_BCN_EN				BIT(4)
#define	TXRPT_CTRL_88E_TXRPT_DIS				BIT(3)
#define	TXRPT_CTRL_88E_CNT_OVER_EN			BIT(2)
#define	TXRPT_CTRL_88E_TXRPT_TIM_EN			BIT(1)
#define	TXRPT_CTRL_88E_TXRPT_EN				BIT(0)


//----------------------------------------------------------------------------
//       8188E REG_88E_TXRPT_STSSET bits				(Offset 0x4F2-4F3, 16 bits)
//----------------------------------------------------------------------------
#define	TXRPT_STSSET_88E_TX_STS_SEL_SHIFT		11
#define	TXRPT_STSSET_88E_TX_STS_SEL_Mask		0x1F
#define	TXRPT_STSSET_88E_TX_STS_CLR				BIT(10)
#define	TXRPT_STSSET_88E_TX_STS_EN				BIT(8)
#define	TXRPT_STSSET_88E_TX_STS_SET				BIT(7)
#define	TXRPT_STSSET_88E_TX_STS_PORT			BIT(6)
#define	TXRPT_STSSET_88E_TX_STS_SUBTPY_SHIFT	2
#define	TXRPT_STSSET_88E_TX_STS_SUBTPY_Mask	0xF
#define	TXRPT_STSSET_88E_TX_STS_TPY_SHIFT		0
#define	TXRPT_STSSET_88E_TX_STS_TPY_Mask		0x3


//----------------------------------------------------------------------------
//       8188E REG_88E_TXRPT_STSVLD bits			(Offset 0x4F4-4F6, 24 bits)
//----------------------------------------------------------------------------
#define	TXRPT_STSVLD_88E_TX_TPY7_VLD		BIT(23)
#define	TXRPT_STSVLD_88E_TX_TPY6_VLD		BIT(22)
#define	TXRPT_STSVLD_88E_TX_TPY5_VLD		BIT(21)
#define	TXRPT_STSVLD_88E_TX_TPY4_VLD		BIT(20)
#define	TXRPT_STSVLD_88E_TX_TPY3_VLD		BIT(19)
#define	TXRPT_STSVLD_88E_TX_TPY2_VLD		BIT(18)
#define	TXRPT_STSVLD_88E_TX_TPY1_VLD		BIT(17)
#define	TXRPT_STSVLD_88E_TX_TPY0_VLD		BIT(16)
#define	TXRPT_STSVLD_88E_TX_BCN7_FAIL		BIT(15)
#define	TXRPT_STSVLD_88E_TX_BCN6_FAIL		BIT(14)
#define	TXRPT_STSVLD_88E_TX_BCN5_FAIL		BIT(13)
#define	TXRPT_STSVLD_88E_TX_BCN4_FAIL		BIT(12)
#define	TXRPT_STSVLD_88E_TX_BCN3_FAIL		BIT(11)
#define	TXRPT_STSVLD_88E_TX_BCN2_FAIL		BIT(10)
#define	TXRPT_STSVLD_88E_TX_BCN1_FAIL		BIT(9)
#define	TXRPT_STSVLD_88E_TX_BCN0_FAIL		BIT(8)
#define	TXRPT_STSVLD_88E_TX_BCN7_OK			BIT(7)
#define	TXRPT_STSVLD_88E_TX_BCN6_OK			BIT(6)
#define	TXRPT_STSVLD_88E_TX_BCN5_OK			BIT(5)
#define	TXRPT_STSVLD_88E_TX_BCN4_OK			BIT(4)
#define	TXRPT_STSVLD_88E_TX_BCN3_OK			BIT(3)
#define	TXRPT_STSVLD_88E_TX_BCN2_OK			BIT(2)
#define	TXRPT_STSVLD_88E_TX_BCN1_OK			BIT(1)
#define	TXRPT_STSVLD_88E_TX_BCN0_OK			BIT(0)


//----------------------------------------------------------------------------
//       8188E REG_88E_TX_STS_INF bits					(Offset 0x4F8-4F9, 16 bits)
//----------------------------------------------------------------------------
#define	TX_STS_INF_88E_TX_STS_INF_EN				BIT(8)
#define	TX_STS_INF_88E_TX_STS_SET_INF			BIT(7)
#define	TX_STS_INF_88E_TX_STS_PORT_INF			BIT(6)
#define	TX_STS_INF_88E_TX_STS_SUBTPY_INF_SHIFT	2
#define	TX_STS_INF_88E_TX_STS_SUBTPY_INF_Mask	0xF
#define	TX_STS_INF_88E_TX_STS_TPY_INF_SHIFT		0
#define	TX_STS_INF_88E_TX_STS_TPY_INF_Mask		0x3


//----------------------------------------------------------------------------
//       8192C EEPROM/EFUSE share register definition.
//----------------------------------------------------------------------------

//====================================================
//			EEPROM/Efuse PG Offset for 88EE/88EU/88ES
//====================================================
#define	EEPROM_TX_PWR_INX_88E				0x10

#define	EEPROM_ChannelPlan_88E				0xB8
#define	EEPROM_XTAL_88E						0xB9
#define	EEPROM_THERMAL_METER_88E			0xBA
#define	EEPROM_IQK_LCK_88E					0xBB

#define	EEPROM_RF_BOARD_OPTION_88E			0xC1
#define	EEPROM_RF_FEATURE_OPTION_88E		0xC2
#define	EEPROM_RF_BT_SETTING_88E				0xC3
#define	EEPROM_VERSION_88E					0xC4
#define	EEPROM_CUSTOMERID_88E				0xC5
#define	EEPROM_RF_ANTENNA_OPT_88E			0xC9

// RTL88EE
#define	EEPROM_MAC_ADDR_88EE				0xD0
#define	EEPROM_VID_88EE						0xD6
#define	EEPROM_DID_88EE						0xD8
#define	EEPROM_SVID_88EE						0xDA
#define	EEPROM_SMID_88EE						0xDC

//RTL88EU
#define	EEPROM_MAC_ADDR_88EU				0xD7
#define	EEPROM_VID_88EU						0xD0
#define	EEPROM_PID_88EU						0xD2

// RTL88ES
#define	EEPROM_MAC_ADDR_88ES				0x11A


//-----------------------------------------------------
//
//	RTL8188E SDIO Configuration
//
//-----------------------------------------------------

// I/O bus domain address mapping
#define SDIO_LOCAL_BASE				0x10250000
#define WLAN_IOREG_BASE				0x10260000
#define FIRMWARE_FIFO_BASE			0x10270000
#define TX_HIQ_BASE				0x10310000
#define TX_MIQ_BASE				0x10320000
#define TX_LOQ_BASE				0x10330000
#define RX_RX0FF_BASE				0x10340000

// SDIO host local register space mapping.
#define SDIO_LOCAL_MSK				0x0FFF
#define WLAN_IOREG_MSK				0x7FFF
#define WLAN_FIFO_MSK		      		0x1FFF	// Aggregation Length[12:0]
#define WLAN_RX0FF_MSK			      	0x0003

#define SDIO_WITHOUT_REF_DEVICE_ID		0	// Without reference to the SDIO Device ID
#define SDIO_LOCAL_DEVICE_ID			0	// 0b[16], 000b[15:13]
#define WLAN_TX_HIQ_DEVICE_ID			4	// 0b[16], 100b[15:13]
#define WLAN_TX_MIQ_DEVICE_ID			5	// 0b[16], 101b[15:13]
#define WLAN_TX_LOQ_DEVICE_ID			6	// 0b[16], 110b[15:13]
#define WLAN_RX0FF_DEVICE_ID			7	// 0b[16], 111b[15:13]
#define WLAN_IOREG_DEVICE_ID			8	// 1b[16]

// SDIO Tx Free Page Index
#define HI_QUEUE_IDX				0
#define MID_QUEUE_IDX				1
#define LOW_QUEUE_IDX				2
#define PUBLIC_QUEUE_IDX			3

#define SDIO_MAX_TX_QUEUE			3		// HIQ, MIQ and LOQ
#define SDIO_MAX_RX_QUEUE			1

#define SDIO_REG_TX_CTRL			0x0000 // SDIO Tx Control
#define SDIO_REG_HIMR				0x0014 // SDIO Host Interrupt Mask
#define SDIO_REG_HISR				0x0018 // SDIO Host Interrupt Service Routine
#define SDIO_REG_HCPWM				0x0019 // HCI Current Power Mode
#define SDIO_REG_RX0_REQ_LEN			0x001C // RXDMA Request Length
#define SDIO_REG_FREE_TXPG			0x0020 // Free Tx Buffer Page
#define SDIO_REG_HCPWM1				0x0024 // HCI Current Power Mode 1
#define SDIO_REG_OQT_FREE_SPACE		0x0025 // OQT Free Space
#define SDIO_REG_HCPWM2				0x0026 // HCI Current Power Mode 2
#define SDIO_REG_HTSFR_INFO			0x0030 // HTSF Informaion
#define SDIO_REG_HRPWM1				0x0080 // HCI Request Power Mode 1
#define SDIO_REG_HRPWM2				0x0082 // HCI Request Power Mode 2
#define SDIO_REG_HPS_CLKR			0x0084 // HCI Power Save Clock
#define SDIO_REG_HSUS_CTRL			0x0086 // SDIO HCI Suspend Control
#define SDIO_REG_HIMR_ON			0x0090 // SDIO Host Extension Interrupt Mask Always
#define SDIO_REG_HISR_ON			0x0091 // SDIO Host Extension Interrupt Status Always

#define SDIO_HIMR_DISABLED			0

// RTL8188E SDIO Host Interrupt Mask Register
#define SDIO_HIMR_RX_REQUEST_MSK		BIT0
#define SDIO_HIMR_AVAL_MSK			BIT1
#define SDIO_HIMR_TXERR_MSK			BIT2
#define SDIO_HIMR_RXERR_MSK			BIT3
#define SDIO_HIMR_TXFOVW_MSK			BIT4
#define SDIO_HIMR_RXFOVW_MSK			BIT5
#define SDIO_HIMR_TXBCNOK_MSK			BIT6
#define SDIO_HIMR_TXBCNERR_MSK			BIT7
#define SDIO_HIMR_BCNERLY_INT_MSK		BIT16
#define SDIO_HIMR_C2HCMD_MSK			BIT17
#define SDIO_HIMR_CPWM1_MSK			BIT18
#define SDIO_HIMR_CPWM2_MSK			BIT19
#define SDIO_HIMR_HSISR_IND_MSK			BIT20
#define SDIO_HIMR_GTINT3_IND_MSK		BIT21
#define SDIO_HIMR_GTINT4_IND_MSK		BIT22
#define SDIO_HIMR_PSTIMEOUT_MSK			BIT23
#define SDIO_HIMR_OCPINT_MSK			BIT24
#define SDIO_HIMR_ATIMEND_MSK			BIT25
#define SDIO_HIMR_ATIMEND_E_MSK			BIT26
#define SDIO_HIMR_CTWEND_MSK			BIT27

//RTL8188E SDIO Specific
#define	SDIO_HIMR_MCU_ERR_MSK			BIT28
#define	SDIO_HIMR_TSF_BIT32_TOGGLE_MSK	BIT29

// SDIO Host Interrupt Service Routine
#define SDIO_HISR_RX_REQUEST			BIT0
#define SDIO_HISR_AVAL				BIT1
#define SDIO_HISR_TXERR				BIT2
#define SDIO_HISR_RXERR				BIT3
#define SDIO_HISR_TXFOVW			BIT4
#define SDIO_HISR_RXFOVW			BIT5
#define SDIO_HISR_TXBCNOK			BIT6
#define SDIO_HISR_TXBCNERR			BIT7
#define SDIO_HISR_BCNERLY_INT			BIT16
#define SDIO_HISR_C2HCMD			BIT17
#define SDIO_HISR_CPWM1				BIT18
#define SDIO_HISR_CPWM2				BIT19
#define SDIO_HISR_HSISR_IND			BIT20
#define SDIO_HISR_GTINT3_IND			BIT21
#define SDIO_HISR_GTINT4_IND			BIT22
#define SDIO_HISR_PSTIMEOUT			BIT23
#define SDIO_HISR_OCPINT			BIT24
#define SDIO_HISR_ATIMEND			BIT25
#define SDIO_HISR_ATIMEND_E			BIT26
#define SDIO_HISR_CTWEND			BIT27

//RTL8188E SDIO Specific
#define	SDIO_HISR_MCU_ERR					BIT28
#define	SDIO_HISR_TSF_BIT32_TOGGLE		BIT29

#define MASK_SDIO_HISR_CLEAR		(SDIO_HISR_TXERR |\
									SDIO_HISR_RXERR |\
									SDIO_HISR_TXFOVW |\
									SDIO_HISR_RXFOVW |\
									SDIO_HISR_TXBCNOK |\
									SDIO_HISR_TXBCNERR |\
									SDIO_HISR_C2HCMD |\
									SDIO_HISR_CPWM1 |\
									SDIO_HISR_CPWM2 |\
									SDIO_HISR_HSISR_IND |\
									SDIO_HISR_GTINT3_IND |\
									SDIO_HISR_GTINT4_IND |\
									SDIO_HISR_PSTIMEOUT |\
									SDIO_HISR_OCPINT)

// SDIO HCI Suspend Control Register
#define HCI_RESUME_PWR_RDY			BIT1
#define HCI_SUS_CTRL				BIT0

// SDIO Tx FIFO related
#define SDIO_TX_FREE_PG_QUEUE			4	// The number of Tx FIFO free page
#define SDIO_TX_FIFO_PAGE_SZ 			128

//-----------------------------------------------------
//
//	0xFE00h ~ 0xFE55h	USB Configuration
//
//-----------------------------------------------------

//2 Special Option
// 0; Use interrupt endpoint to upload interrupt pkt
// 1; Use bulk endpoint to upload interrupt pkt,
#define INT_BULK_SEL					BIT(4)

//========================================================
// General definitions
//========================================================

#define LAST_ENTRY_OF_TX_PKT_BUFFER_88E		176 // 22k 22528 bytes

#ifdef USE_OUT_SRC


/*--------------------------Define Parameters-------------------------------*/


//
// BB-PHY register PMAC 0x100 PHY 0x800 - 0xEFF
// 1. PMAC duplicate register due to connection: RF_Mode, TRxRN, NumOf L-STF
// 2. 0x800/0x900/0xA00/0xC00/0xD00/0xE00
// 3. RF register 0x00-2E
// 4. Bit Mask for BB/RF register
// 5. Other defintion for BB/RF R/W
//


//
// 1. PMAC duplicate register due to connection: RF_Mode, TRxRN, NumOf L-STF
// 1. Page1(0x100)
//
#define		rPMAC_Reset					0x100
#define		rPMAC_TxStart				0x104
#define		rPMAC_TxLegacySIG			0x108
#define		rPMAC_TxHTSIG1				0x10c
#define		rPMAC_TxHTSIG2				0x110
#define		rPMAC_PHYDebug				0x114
#define		rPMAC_TxPacketNum			0x118
#define		rPMAC_TxIdle					0x11c
#define		rPMAC_TxMACHeader0			0x120
#define		rPMAC_TxMACHeader1			0x124
#define		rPMAC_TxMACHeader2			0x128
#define		rPMAC_TxMACHeader3			0x12c
#define		rPMAC_TxMACHeader4			0x130
#define		rPMAC_TxMACHeader5			0x134
#define		rPMAC_TxDataType				0x138
#define		rPMAC_TxRandomSeed			0x13c
#define		rPMAC_CCKPLCPPreamble		0x140
#define		rPMAC_CCKPLCPHeader			0x144
#define		rPMAC_CCKCRC16				0x148
#define		rPMAC_OFDMRxCRC32OK		0x170
#define		rPMAC_OFDMRxCRC32Er		0x174
#define		rPMAC_OFDMRxParityEr			0x178
#define		rPMAC_OFDMRxCRC8Er			0x17c
#define		rPMAC_CCKCRxRC16Er			0x180
#define		rPMAC_CCKCRxRC32Er			0x184
#define		rPMAC_CCKCRxRC32OK			0x188
#define		rPMAC_TxStatus				0x18c

//
// 2. Page2(0x200)
//
// The following two definition are only used for USB interface.
#define		RF_BB_CMD_ADDR				0x02c0	// RF/BB read/write command address.
#define		RF_BB_CMD_DATA				0x02c4	// RF/BB read/write command data.

//
// 3. Page8(0x800)
//
#define		REG_FPGA0_RFMOD				0x800	//RF mode & CCK TxSC // RF BW Setting??

#define		rFPGA0_TxInfo					0x804	// Status report??
#define		REG_FPGA0_PSD_FUNCTION			0x808

#define		REG_FPGA0_TX_GAIN_STAGE			0x80c	// Set TX PWR init gain?

#define		rFPGA0_RFTiming1				0x810	// Useless now
#define		rFPGA0_RFTiming2				0x814

#define		REG_FPGA0_XA_HSSI_PARAMETER1		0x820	// RF 3 wire register
#define		rFPGA0_XA_HSSIParameter2		0x824
#define		REG_FPGA0_XB_HSSI_PARAMETER1		0x828
#define		rFPGA0_XB_HSSIParameter2		0x82c

#define		REG_FPGA0_XA_LSSI_PARAMETER		0x840
#define		REG_FPGA0_XB_LSSI_PARAMETER		0x844

#define		rFPGA0_RFWakeUpParameter	0x850	// Useless now
#define		rFPGA0_RFSleepUpParameter		0x854

#define		rFPGA0_XAB_SwitchControl		0x858	// RF Channel switch
#define		REG_FPGA0_XCD_SWITCH_CONTROL		0x85c

#define		REG_FPGA0_XA_RF_INTERFACE_OE		0x860	// RF Channel switch
#define		REG_FPGA0_XB_RF_INTERFACE_OE		0x864

#define		REG_FPGA0_XAB_RF_INTERFACE_SW		0x870	// RF Interface Software Control
#define		REG_FPGA0_XCD_RF_INTERFACE_SW		0x874

#define		REG_FPGA0_XAB_RF_PARAMETER		0x878	// RF Parameter
#define		rFPGA0_XCD_RFParameter		0x87c

#define		rFPGA0_AnalogParameter1		0x880	// Crystal cap setting RF-R/W protection for parameter4??
#define		rFPGA0_AnalogParameter2		0x884
#define		rFPGA0_AnalogParameter3		0x888
#define		rFPGA0_AdDaClockEn			0x888	// enable ad/da clock1 for dual-phy
#define		REG_FPGA0_ANALOG_PARAMETER4		0x88c

#define		rFPGA0_XA_LSSIReadBack		0x8a0	// Tranceiver LSSI Readback
#define		REG_FPGA0_XB_LSSI_READ_BACK		0x8a4
#define		rFPGA0_XC_LSSIReadBack		0x8a8
#define		rFPGA0_XD_LSSIReadBack		0x8ac

#define		REG_FPGA0_PSD_REPORT				0x8b4	// Useless now
#define		TransceiverA_HSPI_Readback		0x8b8	// Transceiver A HSPI Readback
#define		TransceiverB_HSPI_Readback		0x8bc	// Transceiver B HSPI Readback
#define		rFPGA0_XAB_RFInterfaceRB		0x8e0	// Useless now // RF Interface Readback Value
#define		rFPGA0_XCD_RFInterfaceRB		0x8e4	// Useless now

//
// 4. Page9(0x900)
//
#define		rFPGA1_RFMOD				0x900	//RF mode & OFDM TxSC // RF BW Setting??

#define		REG_FPGA1_TX_BLOCK				0x904	// Useless now
#define		rFPGA1_DebugSelect			0x908	// Useless now
#define		REG_FPGA1_TX_INFO					0x90c	// Useless now // Status report??

//
// 5. PageA(0xA00)
//
// Set Control channel to upper or lower. These settings are required only for 40MHz
#define		rCCK0_System					0xa00

#define		REG_CCK_0_AFE_SETTING				0xa04	// Disable init gain now // Select RX path by RSSI
#define		rCCK0_CCA					0xa08	// Disable init gain now // Init gain

#define		rCCK0_RxAGC1				0xa0c 	//AGC default value, saturation level // Antenna Diversity, RX AGC, LNA Threshold, RX LNA Threshold useless now. Not the same as 90 series
#define		rCCK0_RxAGC2				0xa10 	//AGC & DAGC

#define		rCCK0_RxHP					0xa14

#define		rCCK0_DSPParameter1			0xa18	//Timing recovery & Channel estimation threshold
#define		rCCK0_DSPParameter2			0xa1c	//SQ threshold

#define		rCCK0_TxFilter1				0xa20
#define		rCCK0_TxFilter2				0xa24
#define		rCCK0_DebugPort				0xa28	//debug port and Tx filter3
#define		rCCK0_FalseAlarmReport		0xa2c	//0xa2d	useless now 0xa30-a4f channel report
#define		rCCK0_TRSSIReport         			0xa50
#define		rCCK0_RxReport            			0xa54  //0xa57
#define		rCCK0_FACounterLower      		0xa5c  //0xa5b
#define		rCCK0_FACounterUpper      		0xa58  //0xa5c

//
// PageB(0xB00)
//
#define		REG_PDP_ANT_A      					0xb00  
#define		REG_PDP_ANT_A_4    				0xb04
#define		rPdp_AntA_8    				0xb08
#define		rPdp_AntA_C    				0xb0c
#define		rPdp_AntA_10    				0xb10
#define		rPdp_AntA_14    				0xb14
#define		rPdp_AntA_18    				0xb18
#define		rPdp_AntA_1C    				0xb1c
#define		rPdp_AntA_20    				0xb20
#define		rPdp_AntA_24    				0xb24

#define		REG_CONFIG_PMPD_ANT_A 			0xb28
#define		REG_CONFIG_RAM64X16				0xb2c

#define		rBndA						0xb30
#define		rHssiPar						0xb34

#define		REG_CONFIG_ANT_A 					0xb68
#define		REG_CONFIG_ANT_B 					0xb6c

#define		REG_PDP_ANT_B 					0xb70
#define		REG_PDP_ANT_B_4 					0xb74
#define		rPdp_AntB_8 					0xb78
#define		rPdp_AntB_C 					0xb7c
#define		rPdp_AntB_10 					0xb80
#define		rPdp_AntB_14 					0xb84
#define		rPdp_AntB_18 					0xb88
#define		rPdp_AntB_1C 					0xb8c
#define		rPdp_AntB_20 					0xb90
#define		rPdp_AntB_24 					0xb94

#define		REG_CONFIG_PMPD_ANT_B			0xb98

#define		rBndB						0xba0

#define		REG_APK							0xbd8
#define		rPm_Rx0_AntA				0xbdc
#define		rPm_Rx1_AntA				0xbe0
#define		rPm_Rx2_AntA				0xbe4
#define		rPm_Rx3_AntA				0xbe8
#define		rPm_Rx0_AntB				0xbec
#define		rPm_Rx1_AntB				0xbf0
#define		rPm_Rx2_AntB				0xbf4
#define		rPm_Rx3_AntB				0xbf8



//
// 6. PageC(0xC00)
//
#define		rOFDM0_LSTF					0xc00

#define		REG_OFDM_0_TRX_PATH_ENABLE			0xc04
#define		REG_OFDM_0_TR_MUX_PAR				0xc08
#define		rOFDM0_TRSWIsolation			0xc0c

#define		rOFDM0_XARxAFE				0xc10  //RxIQ DC offset, Rx digital filter, DC notch filter
#define		REG_OFDM_0_XA_RX_IQ_IMBALANCE    		0xc14  //RxIQ imblance matrix
#define		rOFDM0_XBRxAFE            			0xc18
#define		REG_OFDM_0_XB_RX_IQ_IMBALANCE    		0xc1c
#define		rOFDM0_XCRxAFE            			0xc20
#define		rOFDM0_XCRxIQImbalance    		0xc24
#define		rOFDM0_XDRxAFE            			0xc28
#define		rOFDM0_XDRxIQImbalance    		0xc2c

#define		rOFDM0_RxDetector1			0xc30  //PD,BW & SBD	// DM tune init gain
#define		rOFDM0_RxDetector2			0xc34  //SBD & Fame Sync. 
#define		rOFDM0_RxDetector3			0xc38  //Frame Sync.
#define		rOFDM0_RxDetector4			0xc3c  //PD, SBD, Frame Sync & Short-GI

#define		rOFDM0_RxDSP				0xc40  //Rx Sync Path
#define		rOFDM0_CFOandDAGC			0xc44  //CFO & DAGC
#define		rOFDM0_CCADropThreshold		0xc48 //CCA Drop threshold
#define		REG_OFDM_0_ECCA_THRESHOLD			0xc4c // energy CCA

#define		REG_OFDM_0_XA_AGC_CORE1			0xc50	// DIG
#define		rOFDM0_XAAGCCore2			0xc54
#define		REG_OFDM_0_XB_AGC_CORE1			0xc58
#define		rOFDM0_XBAGCCore2			0xc5c
#define		rOFDM0_XCAGCCore1			0xc60
#define		rOFDM0_XCAGCCore2			0xc64
#define		rOFDM0_XDAGCCore1			0xc68
#define		rOFDM0_XDAGCCore2			0xc6c

#define		rOFDM0_AGCParameter1		0xc70
#define		rOFDM0_AGCParameter2		0xc74
#define		REG_OFDM_0_AGC_RSSI_TABLE			0xc78
#define		rOFDM0_HTSTFAGC				0xc7c

#define		REG_OFDM_0_XA_TX_IQ_IMBALANCE		0xc80	// TX PWR TRACK and DIG
#define		rOFDM0_XATxAFE				0xc84
#define		REG_OFDM_0_XB_TX_IQ_IMBALANCE		0xc88
#define		rOFDM0_XBTxAFE				0xc8c
#define		rOFDM0_XCTxIQImbalance		0xc90
#define		REG_OFDM_0_XC_TX_AFE            			0xc94
#define		rOFDM0_XDTxIQImbalance		0xc98
#define		REG_OFDM_0_XD_TX_AFE				0xc9c

#define		REG_OFDM_0_RX_IQ_EXT_ANTA			0xca0
#define		rOFDM0_TxCoeff1				0xca4
#define		rOFDM0_TxCoeff2				0xca8
#define		rOFDM0_TxCoeff3				0xcac
#define		rOFDM0_TxCoeff4				0xcb0
#define		rOFDM0_TxCoeff5				0xcb4
#define		rOFDM0_TxCoeff6				0xcb8
#define		rOFDM0_RxHPParameter		0xce0
#define		rOFDM0_TxPseudoNoiseWgt		0xce4
#define		rOFDM0_FrameSync			0xcf0
#define		rOFDM0_DFSReport			0xcf4


//
// 7. PageD(0xD00)
//
#define		rOFDM1_LSTF					0xd00
#define		rOFDM1_TRxPathEnable			0xd04

#define		rOFDM1_CFO					0xd08	// No setting now
#define		rOFDM1_CSI1					0xd10
#define		rOFDM1_SBD					0xd14
#define		rOFDM1_CSI2					0xd18
#define		rOFDM1_CFOTracking			0xd2c
#define		rOFDM1_TRxMesaure1			0xd34
#define		rOFDM1_IntfDet				0xd3c
#define		rOFDM1_PseudoNoiseStateAB	0xd50
#define		rOFDM1_PseudoNoiseStateCD	0xd54
#define		rOFDM1_RxPseudoNoiseWgt		0xd58

#define		rOFDM_PHYCounter1			0xda0  //cca, parity fail
#define		rOFDM_PHYCounter2			0xda4  //rate illegal, crc8 fail
#define		rOFDM_PHYCounter3			0xda8  //MCS not support

#define		rOFDM_ShortCFOAB			0xdac	// No setting now
#define		rOFDM_ShortCFOCD			0xdb0
#define		rOFDM_LongCFOAB				0xdb4
#define		rOFDM_LongCFOCD				0xdb8
#define		rOFDM_TailCFOAB				0xdbc
#define		rOFDM_TailCFOCD				0xdc0
#define		rOFDM_PWMeasure1          		0xdc4
#define		rOFDM_PWMeasure2          		0xdc8
#define		rOFDM_BWReport				0xdcc
#define		rOFDM_AGCReport				0xdd0
#define		rOFDM_RxSNR				0xdd4
#define		rOFDM_RxEVMCSI				0xdd8
#define		rOFDM_SIGReport				0xddc


//
// 8. PageE(0xE00)
//
#define		REG_TX_AGC_A_RATE18_06			0xe00
#define		REG_TX_AGC_A_RATE54_24			0xe04
#define		REG_TX_AGC_A_CCK_1_MCS32			0xe08
#define		REG_TX_AGC_A_MCS03_MCS00		0xe10
#define		REG_TX_AGC_A_MCS07_MCS04		0xe14
#define		REG_TX_AGC_A_MCS11_MCS08		0xe18
#define		REG_TX_AGC_A_MCS15_MCS12		0xe1c

#define		REG_TX_AGC_B_RATE18_06			0x830
#define		REG_TX_AGC_B_RATE54_24			0x834
#define		REG_TX_AGC_B_CCK_1_55_MCS32		0x838
#define		REG_TX_AGC_B_MCS03_MCS00		0x83c
#define		REG_TX_AGC_B_MCS07_MCS04		0x848
#define		REG_TX_AGC_B_MCS11_MCS08		0x84c
#define		REG_TX_AGC_B_MCS15_MCS12		0x868
#define		REG_TX_AGC_B_CCK_11_A_CCK_2_11		0x86c

#define		REG_FPGA0_IQK					0xe28
#define		REG_TX_IQK_TONE_A				0xe30
#define		REG_RX_IQK_TONE_A				0xe34
#define		REG_TX_IQK_PI_A					0xe38
#define		REG_RX_IQK_PI_A					0xe3c

#define		REG_TX_IQK 						0xe40
#define		REG_RX_IQK						0xe44
#define		REG_IQK_AGC_PTS					0xe48
#define		REG_IQK_AGC_RSP					0xe4c
#define		REG_TX_IQK_TONE_B				0xe50
#define		REG_RX_IQK_TONE_B				0xe54
#define		REG_TX_IQK_PI_B					0xe58
#define		REG_RX_IQK_PI_B					0xe5c
#define		REG_IQK_AGC_CONT				0xe60

#define		REG_BLUE_TOOTH					0xe6c
#define		REG_RX_WAIT_CCA					0xe70
#define		REG_TX_CCK_RFON					0xe74
#define		REG_TX_CCK_BBON				0xe78
#define		REG_TX_OFDM_RFON				0xe7c
#define		REG_TX_OFDM_BBON				0xe80
#define		REG_TX_TO_RX					0xe84
#define		REG_TX_TO_TX					0xe88
#define		REG_RX_CCK						0xe8c

#define		REG_TX_POWER_BEFORE_IQK_A		0xe94
#define		REG_TX_POWER_AFTER_IQK_A			0xe9c

#define		rRx_Power_Before_IQK_A		0xea0
#define		REG_RX_POWER_BEFORE_IQK_A_2		0xea4
#define		rRx_Power_After_IQK_A			0xea8
#define		REG_RX_POWER_AFTER_IQK_A_2		0xeac

#define		REG_TX_POWER_BEFORE_IQK_B		0xeb4
#define		REG_TX_POWER_AFTER_IQK_B			0xebc

#define		rRx_Power_Before_IQK_B		0xec0
#define		REG_RX_POWER_BEFORE_IQK_B_2		0xec4
#define		rRx_Power_After_IQK_B			0xec8
#define		REG_RX_POWER_AFTER_IQK_B_2		0xecc

#define		REG_RX_OFDM					0xed0
#define		REG_RX_WAIT_RIFS 				0xed4
#define		REG_RX_TO_RX 					0xed8
#define		REG_STANDBY 						0xedc
#define		REG_SLEEP 						0xee0
#define		REG_PMPD_ANAEN				0xeec

//
// 7. RF Register 0x00-0x2E (RF 8256)
//    RF-0222D 0x00-3F
//
//Zebra1
#define		rZebra1_HSSIEnable				0x0	// Useless now
#define		rZebra1_TRxEnable1			0x1
#define		rZebra1_TRxEnable2			0x2
#define		rZebra1_AGC					0x4
#define		rZebra1_ChargePump			0x5
#define		rZebra1_Channel				0x7	// RF channel switch

//#endif
#define		rZebra1_TxGain				0x8	// Useless now
#define		rZebra1_TxLPF					0x9
#define		rZebra1_RxLPF					0xb
#define		rZebra1_RxHPFCorner			0xc

//Zebra4
#define		rGlobalCtrl					0	// Useless now
#define		rRTL8256_TxLPF				19
#define		rRTL8256_RxLPF				11

//RTL8258
#define		rRTL8258_TxLPF				0x11	// Useless now
#define		rRTL8258_RxLPF				0x13
#define		rRTL8258_RSSILPF				0xa

//
// RL6052 Register definition
//
#define		RF_AC						0x00	// 

#define		RF_IQADJ_G1					0x01	// 
#define		RF_IQADJ_G2					0x02	// 
#define		RF_BS_PA_APSET_G1_G4		0x03
#define		RF_BS_PA_APSET_G5_G8		0x04
#define		RF_POW_TRSW				0x05	// 

#define		RF_GAIN_RX					0x06	// 
#define		RF_GAIN_TX					0x07	// 

#define		RF_TXM_IDAC					0x08	// 
#define		RF_IPA_G						0x09	// 
#define		RF_TXBIAS_G					0x0A
#define		RF_TXPA_AG					0x0B
#define		RF_IPA_A						0x0C	// 
#define		RF_TXBIAS_A					0x0D
#define		RF_BS_PA_APSET_G9_G11		0x0E
#define		RF_BS_IQGEN					0x0F	// 

#define		RF_MODE1					0x10	// 
#define		RF_MODE2					0x11	// 

#define		RF_RX_AGC_HP				0x12	// 
#define		RF_TX_AGC					0x13	// 
#define		RF_BIAS						0x14	// 
#define		RF_IPA						0x15	// 
#define		RF_TXBIAS					0x16
#define		RF_POW_ABILITY				0x17	// 
#define		RF_CHNLBW					0x18	// RF channel and BW switch
#define		RF_TOP						0x19	// 

#define		RF_RX_G1					0x1A	// 
#define		RF_RX_G2					0x1B	// 

#define		RF_RX_BB2					0x1C	// 
#define		RF_RX_BB1					0x1D	// 

#define		RF_RCK1						0x1E	// 
#define		RF_RCK2						0x1F	// 

#define		RF_TX_G1						0x20	// 
#define		RF_TX_G2						0x21	// 
#define		RF_TX_G3						0x22	// 

#define		RF_TX_BB1					0x23	// 

//#if HARDWARE_TYPE_IS_RTL8192D	== 1
#define		RF_T_METER_92D				0x42	// 
#define		RF_T_METER_88E				0x42	// 
//#else
#define		RF_T_METER					0x24	// 
//#endif

#define		RF_SYN_G1					0x25	// RF TX Power control
#define		RF_SYN_G2					0x26	// RF TX Power control
#define		RF_SYN_G3					0x27	// RF TX Power control
#define		RF_SYN_G4					0x28	// RF TX Power control
#define		RF_SYN_G5					0x29	// RF TX Power control
#define		RF_SYN_G6					0x2A	// RF TX Power control
#define		RF_SYN_G7					0x2B	// RF TX Power control
#define		RF_SYN_G8					0x2C	// RF TX Power control

#define		RF_RCK_OS					0x30	// RF TX PA control

#define		RF_RCK_OS					0x30	// RF TX PA control
#define		RF_TXPA_G1					0x31	// RF TX PA control
#define		RF_TXPA_G2					0x32	// RF TX PA control
#define		RF_TXPA_G3					0x33	// RF TX PA control
#define		RF_TX_BIAS_A				0x35
#define		RF_TX_BIAS_D				0x36
#define		RF_LOBF_9					0x38
#define		RF_RXRF_A3					0x3C	//	
#define		RF_TRSW						0x3F

#define		RF_TXRF_A2					0x41
#define		RF_TXPA_G4					0x46	
#define		RF_TXPA_A4					0x4B	

//
//Bit Mask
//
// 1. Page1(0x100)
#define		bBBResetB					0x100	// Useless now?
#define		bGlobalResetB					0x200
#define		bOFDMTxStart					0x4
#define		bCCKTxStart					0x8
#define		bCRC32Debug					0x100
#define		bPMACLoopback				0x10
#define		bTxLSIG						0xffffff
#define		bOFDMTxRate					0xf
#define		bOFDMTxReserved				0x10
#define		bOFDMTxLength				0x1ffe0
#define		bOFDMTxParity				0x20000
#define		bTxHTSIG1					0xffffff
#define		bTxHTMCSRate				0x7f
#define		bTxHTBW						0x80
#define		bTxHTLength					0xffff00
#define		bTxHTSIG2					0xffffff
#define		bTxHTSmoothing				0x1
#define		bTxHTSounding				0x2
#define		bTxHTReserved				0x4
#define		bTxHTAggreation				0x8
#define		bTxHTSTBC					0x30
#define		bTxHTAdvanceCoding			0x40
#define		bTxHTShortGI					0x80
#define		bTxHTNumberHT_LTF			0x300
#define		bTxHTCRC8					0x3fc00
#define		bCounterReset				0x10000
#define		bNumOfOFDMTx				0xffff
#define		bNumOfCCKTx					0xffff0000
#define		bTxIdleInterval				0xffff
#define		bOFDMService					0xffff0000
#define		bTxMACHeader				0xffffffff
#define		bTxDataInit					0xff
#define		bTxHTMode					0x100
#define		bTxDataType					0x30000
#define		bTxRandomSeed				0xffffffff
#define		bCCKTxPreamble				0x1
#define		bCCKTxSFD					0xffff0000
#define		bCCKTxSIG					0xff
#define		bCCKTxService					0xff00
#define		bCCKLengthExt					0x8000
#define		bCCKTxLength					0xffff0000
#define		bCCKTxCRC16					0xffff
#define		bCCKTxStatus					0x1
#define		bOFDMTxStatus				0x2

#define 		IS_BB_REG_OFFSET_92S(_Offset)		((_Offset >= 0x800) && (_Offset <= 0xfff))

// 2. Page8(0x800)
#define		bRFMOD						0x1	// Reg 0x800 REG_FPGA0_RFMOD
#define		bJapanMode					0x2
#define		bCCKTxSC						0x30
#define		bCCKEn						0x1000000
#define		bOFDMEn					0x2000000

#define		bOFDMRxADCPhase           		0x10000	// Useless now
#define		bOFDMTxDACPhase           		0x40000
#define		bXATxAGC                  				0x3f

#define		bAntennaSelect                 			0x0300

#define		bXBTxAGC                  				0xf00	// Reg 80c REG_FPGA0_TX_GAIN_STAGE
#define		bXCTxAGC                  				0xf000
#define		bXDTxAGC                  				0xf0000
       		
#define		bPAStart                  				0xf0000000	// Useless now
#define		bTRStart                  				0x00f00000
#define		bRFStart                  				0x0000f000
#define		bBBStart                  				0x000000f0
#define		bBBCCKStart               			0x0000000f
#define		bPAEnd                    				0xf          //Reg0x814
#define		bTREnd                    				0x0f000000
#define		bRFEnd                    				0x000f0000
#define		bCCAMask                  				0x000000f0   //T2R
#define		bR2RCCAMask               			0x00000f00
#define		bHSSI_R2TDelay            			0xf8000000
#define		bHSSI_T2RDelay            			0xf80000
#define		bContTxHSSI               			0x400     //chane gain at continue Tx
#define		bIGFromCCK                			0x200
#define		bAGCAddress               			0x3f
#define		bRxHPTx                   				0x7000
#define		bRxHPT2R                  				0x38000
#define		bRxHPCCKIni               			0xc0000
#define		bAGCTxCode                			0xc00000
#define		bAGCRxCode                			0x300000

#define		b3WireDataLength          			0x800	// Reg 0x820~84f REG_FPGA0_XA_HSSI_PARAMETER1
#define		b3WireAddressLength       		0x400

#define		b3WireRFPowerDown         		0x1	// Useless now
//#define bHWSISelect               		0x8
#define		b5GPAPEPolarity           			0x40000000
#define		b2GPAPEPolarity           			0x80000000
#define		bRFSW_TxDefaultAnt        		0x3
#define		bRFSW_TxOptionAnt         		0x30
#define		bRFSW_RxDefaultAnt        		0x300
#define		bRFSW_RxOptionAnt         		0x3000
#define		bRFSI_3WireData           			0x1
#define		bRFSI_3WireClock          			0x2
#define		bRFSI_3WireLoad           			0x4
#define		bRFSI_3WireRW             			0x8
#define		bRFSI_3Wire               			0xf

#define		bRFSI_RFENV               		0x10	// Reg 0x870 REG_FPGA0_XAB_RF_INTERFACE_SW

#define		bRFSI_TRSW                		0x20	// Useless now
#define		bRFSI_TRSWB               		0x40
#define		bRFSI_ANTSW               		0x100
#define		bRFSI_ANTSWB              		0x200
#define		bRFSI_PAPE                			0x400
#define		bRFSI_PAPE5G              		0x800 
#define		bBandSelect               			0x1
#define		bHTSIG2_GI                			0x80
#define		bHTSIG2_Smoothing         		0x01
#define		bHTSIG2_Sounding          		0x02
#define		bHTSIG2_Aggreaton         		0x08
#define		bHTSIG2_STBC              		0x30
#define		bHTSIG2_AdvCoding         		0x40
#define		bHTSIG2_NumOfHTLTF        	0x300
#define		bHTSIG2_CRC8              		0x3fc
#define		bHTSIG1_MCS               		0x7f
#define		bHTSIG1_BandWidth         		0x80
#define		bHTSIG1_HTLength          		0xffff
#define		bLSIG_Rate                			0xf
#define		bLSIG_Reserved            		0x10
#define		bLSIG_Length              		0x1fffe
#define		bLSIG_Parity              			0x20
#define		bCCKRxPhase               		0x4

#define		bLSSIReadAddress          		0x7f800000   // T65 RF

#define		bLSSIReadEdge             		0x80000000   //LSSI "Read" edge signal

#define		bLSSIReadBackData         		0xfffff		// T65 RF

#define		bLSSIReadOKFlag           		0x1000	// Useless now
#define		bCCKSampleRate            		0x8       //0: 44MHz, 1:88MHz       		
#define		bRegulator0Standby        		0x1
#define		bRegulatorPLLStandby      	0x2
#define		bRegulator1Standby        		0x4
#define		bPLLPowerUp               		0x8
#define		bDPLLPowerUp              		0x10
#define		bDA10PowerUp              		0x20
#define		bAD7PowerUp               		0x200
#define		bDA6PowerUp               		0x2000
#define		bXtalPowerUp              		0x4000
#define		b40MDClkPowerUP           	0x8000
#define		bDA6DebugMode             		0x20000
#define		bDA6Swing                 			0x380000

#define		bADClkPhase               		0x4000000	// Reg 0x880 rFPGA0_AnalogParameter1 20/40 CCK support switch 40/80 BB MHZ

#define		b80MClkDelay              		0x18000000	// Useless
#define		bAFEWatchDogEnable        	0x20000000

#define		bXtalCap01                			0xc0000000	// Reg 0x884 rFPGA0_AnalogParameter2 Crystal cap
#define		bXtalCap23                			0x3
#define		bXtalCap92x				0x0f000000
#define 		bXtalCap                			0x0f000000

#define		bIntDifClkEnable          		0x400	// Useless
#define		bExtSigClkEnable         	 	0x800
#define		bBandgapMbiasPowerUp      	0x10000
#define		bAD11SHGain               		0xc0000
#define		bAD11InputRange           		0x700000
#define		bAD11OPCurrent            		0x3800000
#define		bIPathLoopback            		0x4000000
#define		bQPathLoopback            		0x8000000
#define		bAFELoopback              		0x10000000
#define		bDA10Swing                		0x7e0
#define		bDA10Reverse              		0x800
#define		bDAClkSource              		0x1000
#define		bAD7InputRange            		0x6000
#define		bAD7Gain                  			0x38000
#define		bAD7OutputCMMode          	0x40000
#define		bAD7InputCMMode           	0x380000
#define		bAD7Current               		0xc00000
#define		bRegulatorAdjust          		0x7000000
#define		bAD11PowerUpAtTx          	0x1
#define		bDA10PSAtTx               		0x10
#define		bAD11PowerUpAtRx          	0x100
#define		bDA10PSAtRx               		0x1000       		
#define		bCCKRxAGCFormat           		0x200       		
#define		bPSDFFTSamplepPoint       	0xc000
#define		bPSDAverageNum            		0x3000
#define		bIQPathControl            		0xc00
#define		bPSDFreq                  			0x3ff
#define		bPSDAntennaPath           		0x30
#define		bPSDIQSwitch              		0x40
#define		bPSDRxTrigger             		0x400000
#define		bPSDTxTrigger             		0x80000000
#define		bPSDSineToneScale        		0x7f000000
#define		bPSDReport                		0xffff

// 3. Page9(0x900)
#define		bOFDMTxSC                 		0x30000000	// Useless
#define		bCCKTxOn                  			0x1
#define		bOFDMTxOn                 		0x2
#define		bDebugPage                		0xfff  //reset debug page and also HWord, LWord
#define		bDebugItem                		0xff   //reset debug page and LWord
#define		bAntL              	       			0x10
#define		bAntNonHT           	      		0x100
#define		bAntHT1               			0x1000
#define		bAntHT2                   			0x10000
#define		bAntHT1S1                 			0x100000
#define		bAntNonHTS1               		0x1000000

// 4. PageA(0xA00)
#define		bCCKBBMode                		0x3	// Useless
#define		bCCKTxPowerSaving         		0x80
#define		bCCKRxPowerSaving         		0x40

#define		bCCKSideBand              		0x10	// Reg 0xa00 rCCK0_System 20/40 switch

#define		bCCKScramble              		0x8	// Useless
#define		bCCKAntDiversity    		      	0x8000
#define		bCCKCarrierRecovery   	    	0x4000
#define		bCCKTxRate           		     	0x3000
#define		bCCKDCCancel             	 	0x0800
#define		bCCKISICancel             		0x0400
#define		bCCKMatchFilter           		0x0200
#define		bCCKEqualizer             		0x0100
#define		bCCKPreambleDetect       	 	0x800000
#define		bCCKFastFalseCCA          		0x400000
#define		bCCKChEstStart            		0x300000
#define		bCCKCCACount              		0x080000
#define		bCCKcs_lim                			0x070000
#define		bCCKBistMode              		0x80000000
#define		bCCKCCAMask             	  	0x40000000
#define		bCCKTxDACPhase         	   	0x4
#define		bCCKRxADCPhase         	   	0x20000000   //r_rx_clk
#define		bCCKr_cp_mode0         	   	0x0100
#define		bCCKTxDCOffset           	 	0xf0
#define		bCCKRxDCOffset           	 	0xf
#define		bCCKCCAMode              	 	0xc000
#define		bCCKFalseCS_lim           		0x3f00
#define		bCCKCS_ratio              		0xc00000
#define		bCCKCorgBit_sel           		0x300000
#define		bCCKPD_lim                		0x0f0000
#define		bCCKNewCCA                		0x80000000
#define		bCCKRxHPofIG              		0x8000
#define		bCCKRxIG                  			0x7f00
#define		bCCKLNAPolarity           		0x800000
#define		bCCKRx1stGain             		0x7f0000
#define		bCCKRFExtend              		0x20000000 //CCK Rx Iinital gain polarity
#define		bCCKRxAGCSatLevel        	 	0x1f000000
#define		bCCKRxAGCSatCount       	  	0xe0
#define		bCCKRxRFSettle            		0x1f       //AGCsamp_dly
#define		bCCKFixedRxAGC           	 	0x8000
//#define bCCKRxAGCFormat         	 	0x4000   //remove to HSSI register 0x824
#define		bCCKAntennaPolarity      	 	0x2000
#define		bCCKTxFilterType          		0x0c00
#define		bCCKRxAGCReportType   	   	0x0300
#define		bCCKRxDAGCEn              		0x80000000
#define		bCCKRxDAGCPeriod        	  	0x20000000
#define		bCCKRxDAGCSatLevel     	   	0x1f000000
#define		bCCKTimingRecovery       	 	0x800000
#define		bCCKTxC0                  			0x3f0000
#define		bCCKTxC1                  			0x3f000000
#define		bCCKTxC2                  			0x3f
#define		bCCKTxC3                  			0x3f00
#define		bCCKTxC4                  			0x3f0000
#define		bCCKTxC5                  			0x3f000000
#define		bCCKTxC6                  			0x3f
#define		bCCKTxC7                  			0x3f00
#define		bCCKDebugPort             		0xff0000
#define		bCCKDACDebug              		0x0f000000
#define		bCCKFalseAlarmEnable      	0x8000
#define		bCCKFalseAlarmRead        	0x4000
#define		bCCKTRSSI                 			0x7f
#define		bCCKRxAGCReport           		0xfe
#define		bCCKRxReport_AntSel       	0x80000000
#define		bCCKRxReport_MFOff        	0x40000000
#define		bCCKRxRxReport_SQLoss     	0x20000000
#define		bCCKRxReport_Pktloss      	0x10000000
#define		bCCKRxReport_Lockedbit    	0x08000000
#define		bCCKRxReport_RateError    	0x04000000
#define		bCCKRxReport_RxRate       	0x03000000
#define		bCCKRxFACounterLower      	0xff
#define		bCCKRxFACounterUpper      	0xff000000
#define		bCCKRxHPAGCStart          		0xe000
#define		bCCKRxHPAGCFinal          		0x1c00       		
#define		bCCKRxFalseAlarmEnable    	0x8000
#define		bCCKFACounterFreeze       	0x4000       		
#define		bCCKTxPathSel             		0x10000000
#define		bCCKDefaultRxPath         		0xc000000
#define		bCCKOptionRxPath          		0x3000000

// 5. PageC(0xC00)
#define		bNumOfSTF                			0x3	// Useless
#define		bShift_L                 			0xc0
#define		bGI_TH                   			0xc
#define		bRxPathA                 			0x1
#define		bRxPathB                 			0x2
#define		bRxPathC                 			0x4
#define		bRxPathD                 			0x8
#define		bTxPathA                 			0x1
#define		bTxPathB                 			0x2
#define		bTxPathC                 			0x4
#define		bTxPathD                 			0x8
#define		bTRSSIFreq               			0x200
#define		bADCBackoff              			0x3000
#define		bDFIRBackoff             			0xc000
#define		bTRSSILatchPhase         		0x10000
#define		bRxIDCOffset             			0xff
#define		bRxQDCOffset             		0xff00
#define		bRxDFIRMode              		0x1800000
#define		bRxDCNFType              		0xe000000
#define		bRXIQImb_A               		0x3ff
#define		bRXIQImb_B               			0xfc00
#define		bRXIQImb_C               			0x3f0000
#define		bRXIQImb_D               		0xffc00000
#define		bDC_dc_Notch             		0x60000
#define		bRxNBINotch              		0x1f000000
#define		bPD_TH                   			0xf
#define		bPD_TH_Opt2              		0xc000
#define		bPWED_TH                 			0x700
#define		bIfMF_Win_L              		0x800
#define		bPD_Option               			0x1000
#define		bMF_Win_L                			0xe000
#define		bBW_Search_L             		0x30000
#define		bwin_enh_L               			0xc0000
#define		bBW_TH                   			0x700000
#define		bED_TH2                  			0x3800000
#define		bBW_option               			0x4000000
#define		bRatio_TH                			0x18000000
#define		bWindow_L                			0xe0000000
#define		bSBD_Option              		0x1
#define		bFrame_TH                			0x1c
#define		bFS_Option               			0x60
#define		bDC_Slope_check          		0x80
#define		bFGuard_Counter_DC_L     	0xe00
#define		bFrame_Weight_Short      	0x7000
#define		bSub_Tune                			0xe00000
#define		bFrame_DC_Length         		0xe000000
#define		bSBD_start_offset        		0x30000000
#define		bFrame_TH_2              		0x7
#define		bFrame_GI2_TH            		0x38
#define		bGI2_Sync_en             		0x40
#define		bSarch_Short_Early       		0x300
#define		bSarch_Short_Late        		0xc00
#define		bSarch_GI2_Late          		0x70000
#define		bCFOAntSum               		0x1
#define		bCFOAcc                  			0x2
#define		bCFOStartOffset          		0xc
#define		bCFOLookBack             		0x70
#define		bCFOSumWeight            		0x80
#define		bDAGCEnable              		0x10000
#define		bTXIQImb_A               			0x3ff
#define		bTXIQImb_B               			0xfc00
#define		bTXIQImb_C               			0x3f0000
#define		bTXIQImb_D               			0xffc00000
#define		bTxIDCOffset             			0xff
#define		bTxQDCOffset             		0xff00
#define		bTxDFIRMode              		0x10000
#define		bTxPesudoNoiseOn         		0x4000000
#define		bTxPesudoNoise_A         		0xff
#define		bTxPesudoNoise_B         		0xff00
#define		bTxPesudoNoise_C         		0xff0000
#define		bTxPesudoNoise_D         		0xff000000
#define		bCCADropOption           		0x20000
#define		bCCADropThres            		0xfff00000
#define		bEDCCA_H                 			0xf
#define		bEDCCA_L                 			0xf0
#define		bLambda_ED               		0x300
#define		bRxInitialGain           			0x7f
#define		bRxAntDivEn              		0x80
#define		bRxAGCAddressForLNA      	0x7f00
#define		bRxHighPowerFlow         		0x8000
#define		bRxAGCFreezeThres        		0xc0000
#define		bRxFreezeStep_AGC1       	0x300000
#define		bRxFreezeStep_AGC2       	0xc00000
#define		bRxFreezeStep_AGC3       	0x3000000
#define		bRxFreezeStep_AGC0       	0xc000000
#define		bRxRssi_Cmp_En           		0x10000000
#define		bRxQuickAGCEn            		0x20000000
#define		bRxAGCFreezeThresMode    	0x40000000
#define		bRxOverFlowCheckType     	0x80000000
#define		bRxAGCShift              			0x7f
#define		bTRSW_Tri_Only           		0x80
#define		bPowerThres              		0x300
#define		bRxAGCEn                 			0x1
#define		bRxAGCTogetherEn         		0x2
#define		bRxAGCMin                		0x4
#define		bRxHP_Ini                			0x7
#define		bRxHP_TRLNA              		0x70
#define		bRxHP_RSSI               			0x700
#define		bRxHP_BBP1               		0x7000
#define		bRxHP_BBP2               		0x70000
#define		bRxHP_BBP3               		0x700000
#define		bRSSI_H                  			0x7f0000     //the threshold for high power
#define		bRSSI_Gen                			0x7f000000   //the threshold for ant diversity
#define		bRxSettle_TRSW           		0x7
#define		bRxSettle_LNA            		0x38
#define		bRxSettle_RSSI           		0x1c0
#define		bRxSettle_BBP            		0xe00
#define		bRxSettle_RxHP           		0x7000
#define		bRxSettle_AntSW_RSSI     	0x38000
#define		bRxSettle_AntSW          		0xc0000
#define		bRxProcessTime_DAGC      	0x300000
#define		bRxSettle_HSSI           		0x400000
#define		bRxProcessTime_BBPPW     	0x800000
#define		bRxAntennaPowerShift     	0x3000000
#define		bRSSITableSelect         		0xc000000
#define		bRxHP_Final              			0x7000000
#define		bRxHTSettle_BBP          		0x7
#define		bRxHTSettle_HSSI         		0x8
#define		bRxHTSettle_RxHP         		0x70
#define		bRxHTSettle_BBPPW        		0x80
#define		bRxHTSettle_Idle         		0x300
#define		bRxHTSettle_Reserved     	0x1c00
#define		bRxHTRxHPEn              		0x8000
#define		bRxHTAGCFreezeThres      	0x30000
#define		bRxHTAGCTogetherEn       	0x40000
#define		bRxHTAGCMin             	 	0x80000
#define		bRxHTAGCEn               		0x100000
#define		bRxHTDAGCEn              		0x200000
#define		bRxHTRxHP_BBP            		0x1c00000
#define		bRxHTRxHP_Final          		0xe0000000
#define		bRxPWRatioTH             		0x3
#define		bRxPWRatioEn             		0x4
#define		bRxMFHold                			0x3800
#define		bRxPD_Delay_TH1          		0x38
#define		bRxPD_Delay_TH2          		0x1c0
#define		bRxPD_DC_COUNT_MAX       	0x600
//#define bRxMF_Hold               0x3800
#define		bRxPD_Delay_TH           		0x8000
#define		bRxProcess_Delay         		0xf0000
#define		bRxSearchrange_GI2_Early 	0x700000
#define		bRxFrame_Guard_Counter_L 	0x3800000
#define		bRxSGI_Guard_L           		0xc000000
#define		bRxSGI_Search_L          		0x30000000
#define		bRxSGI_TH                			0xc0000000
#define		bDFSCnt0                 			0xff
#define		bDFSCnt1                 			0xff00
#define		bDFSFlag                 			0xf0000       		
#define		bMFWeightSum             		0x300000
#define		bMinIdxTH                			0x7f000000       		
#define		bDAFormat                			0x40000       		
#define		bTxChEmuEnable           		0x01000000       		
#define		bTRSWIsolation_A         		0x7f
#define		bTRSWIsolation_B         		0x7f00
#define		bTRSWIsolation_C         		0x7f0000
#define		bTRSWIsolation_D         		0x7f000000       		
#define		bExtLNAGain              		0x7c00          

// 6. PageE(0xE00)
#define		bSTBCEn                  			0x4	// Useless
#define		bAntennaMapping          		0x10
#define		bNss                     			0x20
#define		bCFOAntSumD              		0x200
#define		bPHYCounterReset         		0x8000000
#define		bCFOReportGet            		0x4000000
#define		bOFDMContinueTx          		0x10000000
#define		bOFDMSingleCarrier       		0x20000000
#define		bOFDMSingleTone          		0x40000000
//#define bRxPath1                 0x01
//#define bRxPath2                 0x02
//#define bRxPath3                 0x04
//#define bRxPath4                 0x08
//#define bTxPath1                 0x10
//#define bTxPath2                 0x20
#define		bHTDetect                			0x100
#define		bCFOEn                   			0x10000
#define		bCFOValue                			0xfff00000
#define		bSigTone_Re              			0x3f
#define		bSigTone_Im              			0x7f00
#define		bCounter_CCA             		0xffff
#define		bCounter_ParityFail      		0xffff0000
#define		bCounter_RateIllegal     		0xffff
#define		bCounter_CRC8Fail        		0xffff0000
#define		bCounter_MCSNoSupport    	0xffff
#define		bCounter_FastSync        		0xffff
#define		bShortCFO                			0xfff
#define		bShortCFOTLength         		12   //total
#define		bShortCFOFLength         		11   //fraction
#define		bLongCFO                 			0x7ff
#define		bLongCFOTLength          		11
#define		bLongCFOFLength          		11
#define		bTailCFO                 			0x1fff
#define		bTailCFOTLength          		13
#define		bTailCFOFLength          		12       		
#define		bmax_en_pwdB             		0xffff
#define		bCC_power_dB             		0xffff0000
#define		bnoise_pwdB              		0xffff
#define		bPowerMeasTLength        	10
#define		bPowerMeasFLength        	3
#define		bRx_HT_BW                		0x1
#define		bRxSC                    			0x6
#define		bRx_HT                   			0x8       		
#define		bNB_intf_det_on          		0x1
#define		bIntf_win_len_cfg        		0x30
#define		bNB_Intf_TH_cfg          		0x1c0       		
#define		bRFGain                  			0x3f
#define		bTableSel                			0x40
#define		bTRSW                    			0x80       		
#define		bRxSNR_A                 			0xff
#define		bRxSNR_B                 			0xff00
#define		bRxSNR_C                 			0xff0000
#define		bRxSNR_D                 			0xff000000
#define		bSNREVMTLength           		8
#define		bSNREVMFLength           		1       		
#define		bCSI1st                  			0xff
#define		bCSI2nd                  			0xff00
#define		bRxEVM1st                			0xff0000
#define		bRxEVM2nd                		0xff000000       		
#define		bSIGEVM                  			0xff
#define		bPWDB                    			0xff00
#define		bSGIEN                   			0x10000
       		
#define		bSFactorQAM1             		0xf	// Useless
#define		bSFactorQAM2             		0xf0
#define		bSFactorQAM3             		0xf00
#define		bSFactorQAM4             		0xf000
#define		bSFactorQAM5             		0xf0000
#define		bSFactorQAM6             		0xf0000
#define		bSFactorQAM7             		0xf00000
#define		bSFactorQAM8             		0xf000000
#define		bSFactorQAM9             		0xf0000000
#define		bCSIScheme               			0x100000
       		
#define		bNoiseLvlTopSet          		0x3	// Useless
#define		bChSmooth                			0x4
#define		bChSmoothCfg1            		0x38
#define		bChSmoothCfg2            		0x1c0
#define		bChSmoothCfg3            		0xe00
#define		bChSmoothCfg4            		0x7000
#define		bMRCMode                 		0x800000
#define		bTHEVMCfg                			0x7000000
       		
#define		bLoopFitType             			0x1	// Useless
#define		bUpdCFO                  			0x40
#define		bUpdCFOOffData           		0x80
#define		bAdvUpdCFO               		0x100
#define		bAdvTimeCtrl             		0x800
#define		bUpdClko                 			0x1000
#define		bFC                      				0x6000
#define		bTrackingMode            		0x8000
#define		bPhCmpEnable             		0x10000
#define		bUpdClkoLTF              			0x20000
#define		bComChCFO                		0x40000
#define		bCSIEstiMode             		0x80000
#define		bAdvUpdEqz               		0x100000
#define		bUChCfg                  			0x7000000
#define		bUpdEqz                  			0x8000000

//Rx Pseduo noise
#define		bRxPesudoNoiseOn         		0x20000000	// Useless
#define		bRxPesudoNoise_A         		0xff
#define		bRxPesudoNoise_B         		0xff00
#define		bRxPesudoNoise_C         		0xff0000
#define		bRxPesudoNoise_D         		0xff000000
#define		bPesudoNoiseState_A      	0xffff
#define		bPesudoNoiseState_B      	0xffff0000
#define		bPesudoNoiseState_C      		0xffff
#define		bPesudoNoiseState_D      	0xffff0000

//7. RF Register
//Zebra1
#define		bZebra1_HSSIEnable        		0x8		// Useless
#define		bZebra1_TRxControl        		0xc00
#define		bZebra1_TRxGainSetting    	0x07f
#define		bZebra1_RxCorner          		0xc00
#define		bZebra1_TxChargePump      	0x38
#define		bZebra1_RxChargePump      	0x7
#define		bZebra1_ChannelNum        	0xf80
#define		bZebra1_TxLPFBW           		0x400
#define		bZebra1_RxLPFBW           		0x600

//Zebra4
#define		bRTL8256RegModeCtrl1      	0x100	// Useless
#define		bRTL8256RegModeCtrl0      	0x40
#define		bRTL8256_TxLPFBW          	0x18
#define		bRTL8256_RxLPFBW          	0x600

//RTL8258
#define		bRTL8258_TxLPFBW          	0xc	// Useless
#define		bRTL8258_RxLPFBW          	0xc00
#define		bRTL8258_RSSILPFBW        	0xc0


//
// Other Definition
//

//byte endable for sb_write
#define		bByte0                    			0x1	// Useless
#define		bByte1                    			0x2
#define		bByte2                    			0x4
#define		bByte3                    			0x8
#define		bWord0                    			0x3
#define		bWord1                    			0xc
#define		bDWord                    			0xf

//for PutRegsetting & GetRegSetting BitMask
#define		bMaskByte0                		0xff	// Reg 0xc50 rOFDM0_XAAGCCore~0xC6f
#define		bMaskByte1                		0xff00
#define		bMaskByte2                		0xff0000
#define		bMaskByte3                		0xff000000
#define		bMaskHWord                		0xffff0000
#define		bMaskLWord                		0x0000ffff
#define		bMaskDWord                		0xffffffff
#define		bMask12Bits				0xfff	
#define		bMaskH4Bits				0xf0000000	
#define		bMaskH3Bytes			0xffffff00	
#define 		bMaskOFDM_D			0xffc00000
#define		bMaskCCK				0x3f3f3f3f

//for PutRFRegsetting & GetRFRegSetting BitMask
//#define		bMask12Bits               0xfffff	// RF Reg mask bits
//#define		bMask20Bits               0xfffff	// RF Reg mask bits T65 RF
#define 		bRFRegOffsetMask			0xfffff		
  		
//#define		bEnable                   0x1	// Useless
//#define		bDisable                  0x0
       		
#define		LeftAntenna               			0x0	// Useless
#define		RightAntenna              		0x1
       		
#define		tCheckTxStatus            		500   //500ms // Useless
#define		tUpdateRxCounter          		100   //100ms
       		
#define		rateCCK     				0	// Useless
#define		rateOFDM    				1
#define		rateHT      					2

//define Register-End
#define		bPMAC_End                 		0x1ff	// Useless
#define		bFPGAPHY0_End             		0x8ff
#define		bFPGAPHY1_End             		0x9ff
#define		bCCKPHY0_End              		0xaff
#define		bOFDMPHY0_End             		0xcff
#define		bOFDMPHY1_End             		0xdff

//define max debug item in each debug page
//#define bMaxItem_FPGA_PHY0        0x9
//#define bMaxItem_FPGA_PHY1        0x3
//#define bMaxItem_PHY_11B          0x16
//#define bMaxItem_OFDM_PHY0        0x29
//#define bMaxItem_OFDM_PHY1        0x0

#define		bPMACControl              		0x0		// Useless
#define		bWMACControl              		0x1
#define		bWNICControl              		0x2
       		
#define		PathA                     			0x0	// Useless
#define		PathB                     			0x1
#define		PathC                     			0x2
#define		PathD                     			0x3

/*--------------------------Define Parameters-------------------------------*/
#endif

#endif //#ifndef WLAN_HAL_INTERNAL_USED

#endif

