/*
 *  SDIO core routines
 *
 *  Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8188E_SDIO_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#endif

#include "8192cd.h"
#include "8192cd_headers.h"
#include "8192cd_debug.h"

const u32 reg_freepage_thres[SDIO_TX_FREE_PG_QUEUE] = {
	REG_88E_TXDMA_TH+1, REG_88E_TXDMA_TH+3, REG_88E_LQ_TH+1, 0
};


//
// Description:
//	The following mapping is for SDIO host local register space.
//
// Creadted by Roger, 2011.01.31.
//
void HalSdioGetCmdAddr8723ASdio(struct rtl8192cd_priv *priv, u8 DeviceID, u32 Addr, u32* pCmdAddr)
{
	switch (DeviceID)
	{
		case SDIO_LOCAL_DEVICE_ID:
			*pCmdAddr = ((SDIO_LOCAL_DEVICE_ID << 13) | (Addr & SDIO_LOCAL_MSK));
			break;

		case WLAN_IOREG_DEVICE_ID:
			*pCmdAddr = ((WLAN_IOREG_DEVICE_ID << 13) | (Addr & WLAN_IOREG_MSK));
			break;

		case WLAN_TX_HIQ_DEVICE_ID:
			*pCmdAddr = ((WLAN_TX_HIQ_DEVICE_ID << 13) | (Addr & WLAN_FIFO_MSK));
			break;

		case WLAN_TX_MIQ_DEVICE_ID:
			*pCmdAddr = ((WLAN_TX_MIQ_DEVICE_ID << 13) | (Addr & WLAN_FIFO_MSK));
			break;

		case WLAN_TX_LOQ_DEVICE_ID:
			*pCmdAddr = ((WLAN_TX_LOQ_DEVICE_ID << 13) | (Addr & WLAN_FIFO_MSK));
			break;

		case WLAN_RX0FF_DEVICE_ID:
			*pCmdAddr = ((WLAN_RX0FF_DEVICE_ID << 13) | (Addr & WLAN_RX0FF_MSK));
			break;

		default:
			break;
	}
}

u8 get_deviceid(u32 addr)
{
	u32 baseAddr;
	u8 devideId;

	baseAddr = addr & 0xffff0000;
	
	switch (baseAddr) {
	case SDIO_LOCAL_BASE:
		devideId = SDIO_LOCAL_DEVICE_ID;
		break;

	case WLAN_IOREG_BASE:
		devideId = WLAN_IOREG_DEVICE_ID;
		break;

//	case FIRMWARE_FIFO_BASE:
//		devideId = SDIO_FIRMWARE_FIFO;
//		break;

	case TX_HIQ_BASE:
		devideId = WLAN_TX_HIQ_DEVICE_ID;
		break;

	case TX_MIQ_BASE:
		devideId = WLAN_TX_MIQ_DEVICE_ID;
		break;

	case TX_LOQ_BASE:
		devideId = WLAN_TX_LOQ_DEVICE_ID;
		break;

	case RX_RX0FF_BASE:
		devideId = WLAN_RX0FF_DEVICE_ID;
		break;

	default:
//		devideId = (u8)((addr >> 13) & 0xF);
		devideId = WLAN_IOREG_DEVICE_ID;
		break;
	}

	return devideId;
}

/*
 * Ref:
 *	HalSdioGetCmdAddr8723ASdio()
 */
u32 _cvrt2ftaddr(const u32 addr, u8 *pdeviceId, u16 *poffset)
{
	u8 deviceId;
	u16 offset;
	u32 ftaddr;


	deviceId = get_deviceid(addr);
	offset = 0;

	switch (deviceId)
	{
		case SDIO_LOCAL_DEVICE_ID:
			offset = addr & SDIO_LOCAL_MSK;
			break;

		case WLAN_TX_HIQ_DEVICE_ID:
		case WLAN_TX_MIQ_DEVICE_ID:
		case WLAN_TX_LOQ_DEVICE_ID:
			offset = addr & WLAN_FIFO_MSK;
			break;

		case WLAN_RX0FF_DEVICE_ID:
			offset = addr & WLAN_RX0FF_MSK;
			break;

		case WLAN_IOREG_DEVICE_ID:
		default:
			deviceId = WLAN_IOREG_DEVICE_ID;
			offset = addr & WLAN_IOREG_MSK;
			break;
	}
	ftaddr = (deviceId << 13) | offset;

	if (pdeviceId) *pdeviceId = deviceId;
	if (poffset) *poffset = offset;

	return ftaddr;
}


/*
 *
 *    HAL
 *
 */
 
void InitSdioInterrupt(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData;
	
	pHalData = GET_HAL_INTF_DATA(priv);
	pHalData->sdio_himr = SDIO_HIMR_RX_REQUEST_MSK
#ifdef CONFIG_SDIO_TX_INTERRUPT
			| SDIO_HIMR_AVAL_MSK
#endif
			;
}
 
void EnableSdioInterrupt(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData;
	u32 himr;

	pHalData = GET_HAL_INTF_DATA(priv);
	himr = cpu_to_le32(pHalData->sdio_himr);
	sdio_local_write(priv, SDIO_REG_HIMR, 4, (u8*)&himr);

	//
	// <Roger_Notes> There are some C2H CMDs have been sent before system interrupt is enabled, e.g., C2H, CPWM.
	// So we need to clear all C2H events that FW has notified, otherwise FW won't schedule any commands anymore.
	// 2011.10.19.
	//
	RTL_W8(C2H_SYNC_BYTE, C2H_EVT_HOST_CLOSE);
}

void DisableSdioInterrupt(struct rtl8192cd_priv *priv)
{
	u32 himr;

	himr = cpu_to_le32(SDIO_HIMR_DISABLED);
	sdio_local_write(priv, SDIO_REG_HIMR, 4, (u8*)&himr);
}

#ifdef CONFIG_POWER_SAVE
void ClearSdioInterrupt(struct rtl8192cd_priv *priv)
{
	// clear HISR
	u32 hisr = cpu_to_le32(MASK_SDIO_HISR_CLEAR);
	sdio_local_write(priv, SDIO_REG_HISR, 4, (u8*)&hisr);
}

void set_ap_32k(struct rtl8192cd_priv *priv, BOOLEAN en_32K)
{
	u1Byte rpwm;
	u1Byte cpwm, cpwm_org;
	u1Byte wait_times = 0;

	cpwm_org = SdioLocalCmd52Read1Byte(priv, SDIO_REG_HCPWM1);

	/* set rpwm */
	rpwm = SdioLocalCmd52Read1Byte(priv, SDIO_REG_HRPWM1);
	if (en_32K) {
		rpwm = rpwm ^ BIT7;	//toggle
		rpwm = rpwm & ~BIT6;	//clear fw ack
		rpwm = rpwm | BIT0;	//enter
	} else {
		rpwm = rpwm ^ BIT7;	//toggle
		rpwm = rpwm | BIT6;	//need fw ack
		rpwm = rpwm & ~BIT0;	//leave
	}
	SdioLocalCmd52Write1Byte(priv, SDIO_REG_HRPWM1, rpwm);

	if (en_32K)
		return;

	/* polling cpwm ack bit */
	while (wait_times < 50) {
		cpwm = SdioLocalCmd52Read1Byte(priv, SDIO_REG_HCPWM1);
		//printk("[%s %d] cpwm=0x%02x, wait_times=%2d, 0x90=0x02X\n",
		//	__FUNCTION__, __LINE__, cpwm, wait_times, RTL_R8(0x90));
		if ((cpwm ^ cpwm_org) & BIT7)
			break;
		udelay(500);
		wait_times++;
	}

	if (wait_times >= 50)
	{
		priv->pshare->nr_leave_32k_fail++;
		printk(KERN_ERR "[%s %d] leave 32K fail!!(cpwm=0x%x)\n", __FUNCTION__, __LINE__, cpwm);
	}
	else
	{
		DEBUG_INFO("[%s %d] leave 32K success!\n", __FUNCTION__, __LINE__);
	}
}
#endif

static void _OneOutPipeMapping(HAL_INTF_DATA_TYPE *pHalData)
{
	pHalData->Queue2Pipe[VO_QUEUE] = pHalData->RtOutPipe[0];//VO
	pHalData->Queue2Pipe[VI_QUEUE] = pHalData->RtOutPipe[0];//VI
	pHalData->Queue2Pipe[BE_QUEUE] = pHalData->RtOutPipe[0];//BE
	pHalData->Queue2Pipe[BK_QUEUE] = pHalData->RtOutPipe[0];//BK
	
	pHalData->Queue2Pipe[BEACON_QUEUE] = pHalData->RtOutPipe[0];//BCN
	pHalData->Queue2Pipe[MGNT_QUEUE] = pHalData->RtOutPipe[0];//MGT
	pHalData->Queue2Pipe[HIGH_QUEUE] = pHalData->RtOutPipe[0];//HIGH
	pHalData->Queue2Pipe[TXCMD_QUEUE] = pHalData->RtOutPipe[0];//TXCMD
}

static void _TwoOutPipeMapping(HAL_INTF_DATA_TYPE *pHalData, BOOLEAN bWIFICfg)
{
	if (bWIFICfg) { //WMM

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  0, 	1, 	0, 	1, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:L

		pHalData->Queue2Pipe[VO_QUEUE] = pHalData->RtOutPipe[1];//VO
		pHalData->Queue2Pipe[VI_QUEUE] = pHalData->RtOutPipe[0];//VI
		pHalData->Queue2Pipe[BE_QUEUE] = pHalData->RtOutPipe[1];//BE
		pHalData->Queue2Pipe[BK_QUEUE] = pHalData->RtOutPipe[0];//BK

		pHalData->Queue2Pipe[BEACON_QUEUE] = pHalData->RtOutPipe[0];//BCN
		pHalData->Queue2Pipe[MGNT_QUEUE] = pHalData->RtOutPipe[0];//MGT
		pHalData->Queue2Pipe[HIGH_QUEUE] = pHalData->RtOutPipe[0];//HIGH
		pHalData->Queue2Pipe[TXCMD_QUEUE] = pHalData->RtOutPipe[0];//TXCMD

	}
	else{//typical setting

		//BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  1, 	1, 	0, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:L

		pHalData->Queue2Pipe[VO_QUEUE] = pHalData->RtOutPipe[0];//VO
		pHalData->Queue2Pipe[VI_QUEUE] = pHalData->RtOutPipe[0];//VI
		pHalData->Queue2Pipe[BE_QUEUE] = pHalData->RtOutPipe[1];//BE
		pHalData->Queue2Pipe[BK_QUEUE] = pHalData->RtOutPipe[1];//BK

		pHalData->Queue2Pipe[BEACON_QUEUE] = pHalData->RtOutPipe[0];//BCN
		pHalData->Queue2Pipe[MGNT_QUEUE] = pHalData->RtOutPipe[0];//MGT
		pHalData->Queue2Pipe[HIGH_QUEUE] = pHalData->RtOutPipe[0];//HIGH
		pHalData->Queue2Pipe[TXCMD_QUEUE] = pHalData->RtOutPipe[0];//TXCMD

	}
	
}

static void _ThreeOutPipeMapping(HAL_INTF_DATA_TYPE *pHalData, BOOLEAN bWIFICfg)
{
	if (bWIFICfg) {//for WMM

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L

		pHalData->Queue2Pipe[VO_QUEUE] = pHalData->RtOutPipe[0];//VO
		pHalData->Queue2Pipe[VI_QUEUE] = pHalData->RtOutPipe[1];//VI
		pHalData->Queue2Pipe[BE_QUEUE] = pHalData->RtOutPipe[2];//BE
		pHalData->Queue2Pipe[BK_QUEUE] = pHalData->RtOutPipe[1];//BK

		pHalData->Queue2Pipe[BEACON_QUEUE] = pHalData->RtOutPipe[0];//BCN
		pHalData->Queue2Pipe[MGNT_QUEUE] = pHalData->RtOutPipe[0];//MGT
		pHalData->Queue2Pipe[HIGH_QUEUE] = pHalData->RtOutPipe[0];//HIGH
		pHalData->Queue2Pipe[TXCMD_QUEUE] = pHalData->RtOutPipe[0];//TXCMD

	}
	else{//typical setting

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L

		pHalData->Queue2Pipe[VO_QUEUE] = pHalData->RtOutPipe[0];//VO
		pHalData->Queue2Pipe[VI_QUEUE] = pHalData->RtOutPipe[1];//VI
		pHalData->Queue2Pipe[BE_QUEUE] = pHalData->RtOutPipe[2];//BE
		pHalData->Queue2Pipe[BK_QUEUE] = pHalData->RtOutPipe[2];//BK

		pHalData->Queue2Pipe[BEACON_QUEUE] = pHalData->RtOutPipe[0];//BCN
		pHalData->Queue2Pipe[MGNT_QUEUE] = pHalData->RtOutPipe[0];//MGT
		pHalData->Queue2Pipe[HIGH_QUEUE] = pHalData->RtOutPipe[0];//HIGH
		pHalData->Queue2Pipe[TXCMD_QUEUE] = pHalData->RtOutPipe[0];//TXCMD
	}

}

static BOOLEAN _MappingOutPipe(struct rtl8192cd_priv *priv, u8 NumOutPipe)
{
	HAL_INTF_DATA_TYPE* pHalData = GET_HAL_INTF_DATA(priv);
	BOOLEAN	 bWIFICfg = (priv->pmib->dot11OperationEntry.wifi_specific) ? TRUE : FALSE;
	
	BOOLEAN result = TRUE;

	switch(NumOutPipe)
	{
		case 2:
			_TwoOutPipeMapping(pHalData, bWIFICfg);
			break;
		case 3:
			_ThreeOutPipeMapping(pHalData, bWIFICfg);
			break;
		case 1:
			_OneOutPipeMapping(pHalData);
			break;
		default:
			result = FALSE;
			break;
	}

	return result;
	
}


void rtl8188es_interface_configure(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	BOOLEAN	 bWiFiConfig = (priv->pmib->dot11OperationEntry.wifi_specific) ? TRUE : FALSE;


	pHalData->RtOutPipe[0] = WLAN_TX_HIQ_DEVICE_ID;
	pHalData->RtOutPipe[1] = WLAN_TX_MIQ_DEVICE_ID;
	pHalData->RtOutPipe[2] = WLAN_TX_LOQ_DEVICE_ID;

	if (bWiFiConfig)
		pHalData->OutEpNumber = 2;
	else
		pHalData->OutEpNumber = SDIO_MAX_TX_QUEUE;

	switch(pHalData->OutEpNumber){
		case 3:
			pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_LQ|TX_SELE_NQ;
			break;
		case 2:
			pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_NQ;
			break;
		case 1:
			pHalData->OutEpQueueSel=TX_SELE_HQ;
			break;
		default:
			break;
	}

	// The QUEUE-to-OUT Pipe mapping must match _InitQueuePriority() settings
	_MappingOutPipe(priv, pHalData->OutEpNumber);
}

#ifdef CONFIG_SDIO_TX_IN_INTERRUPT
s32 sd_tx_rx_hdl(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	struct priv_shared_info *pshare = priv->pshare;
	
	struct recv_buf *precvbuf = NULL;
	s32 err = 0;
	int nr_recvbuf = 0;
	int nr_xmitbuf = 0;
	int nr_handle_recvbuf = 0;
	int loop;
	int handle_tx;
	
	u32 buf[2];
	u8 *data = (u8*) buf;
	u32 sdio_hisr;
	
	do {
		if ((TRUE == pshare->bDriverStopped) || (TRUE == pshare->bSurpriseRemoved))
			break;
		
		err = _sdio_local_read(priv, SDIO_REG_HISR, 8, data);
		if (err)
			break;
		
		sdio_hisr = le32_to_cpu(*(u32*)data);
		if (sdio_hisr & SDIO_HISR_RX_REQUEST)
			pHalData->SdioRxFIFOSize = le16_to_cpu(*(u16*)&data[4]);
		
		handle_tx = 0;
		
		if (pHalData->SdioTxIntStatus & BIT(SDIO_TX_INT_SETUP_TH))
		{
			if (sdio_hisr & SDIO_HISR_AVAL)
			{
				if (test_and_clear_bit(SDIO_TX_INT_SETUP_TH, &pHalData->SdioTxIntStatus)) {
					set_bit(SDIO_TX_INT_WORKING, &pHalData->SdioTxIntStatus);
					
					// Invalidate TX Free Page Threshold
					RTL_W8(reg_freepage_thres[pHalData->SdioTxIntQIdx], 0xFF);
					
					sdio_query_txbuf_status_locksafe(priv);
					
					for (loop = priv->pmib->miscEntry.max_handle_xmitbuf; loop > 0; --loop) {
						if (rtl8188es_dequeue_writeport(priv, SDIO_TX_ISR) == FAIL)
							break;
						++nr_xmitbuf;
					}
					++handle_tx;
				}
			}
		}
#ifdef SDIO_AP_OFFLOAD
		else if (pshare->offload_function_ctrl)
		{
			// do nothing. This purpose is to stop submitting any packet in AP offload (PS) state.
		}
#endif
		else {
			if ((0 == pHalData->SdioRxFIFOSize) || (nr_handle_recvbuf >= priv->pmib->miscEntry.max_handle_recvbuf))
			if (!(pshare->xmit_thread_state & XMIT_THREAD_STATE_RUNNING)
					&& (pHalData->SdioTxIntStatus & BIT(SDIO_TX_INT_WORKING))
					&& (_rtw_queue_empty(&pshare->pending_xmitbuf_queue) == FALSE))
			{
				if (!pHalData->WaitSdioTxOQT
						|| (sdio_query_txoqt_status(priv)
						&& (pHalData->SdioTxOQTFreeSpace >= pHalData->WaitSdioTxOQTSpace))) {
					for (loop = priv->pmib->miscEntry.max_handle_xmitbuf; loop > 0; --loop) {
						if (rtl8188es_dequeue_writeport(priv, SDIO_RX_ISR) == FAIL)
							break;
						++nr_xmitbuf;
					}
					++handle_tx;
					nr_handle_recvbuf = 0;
				}
			}
		}

		if (pHalData->SdioRxFIFOSize)
		{
			if (pHalData->SdioRxFIFOSize > MAX_RECVBUF_SZ) {
				// exception case. read sdio register again.
				printk("[%s %d] sdio_hisr=0x%X, SdioRxFIFOSize=%d (>%d)\n",
					__func__, __LINE__, sdio_hisr, pHalData->SdioRxFIFOSize, MAX_RECVBUF_SZ);
				
				err = _sdio_local_read(priv, SDIO_REG_HISR, 8, data);
				if (err)
					break;
				sdio_hisr = le32_to_cpu(*(u32*)data);
				pHalData->SdioRxFIFOSize = le16_to_cpu(*(u16*)&data[4]);
				printk("[%s %d] sdio_hisr=0x%X, SdioRxFIFOSize=%d\n",
					__func__, __LINE__, sdio_hisr, pHalData->SdioRxFIFOSize);
			}
			
			precvbuf = sd_recv_rxfifo(priv, pHalData->SdioRxFIFOSize);
			if (precvbuf) {
				nr_recvbuf++;
				nr_handle_recvbuf++;
				sd_rxhandler(priv, precvbuf);
			}
			
			pHalData->SdioRxFIFOSize = 0;
		} else if (0 == handle_tx) {
			break;
		}
	} while (1);
	
	if (test_and_clear_bit(SDIO_TX_INT_WORKING, &pHalData->SdioTxIntStatus)
			&& !(pHalData->SdioTxIntStatus & BIT(SDIO_TX_INT_SETUP_TH))) {
		if (_rtw_queue_empty(&pshare->pending_xmitbuf_queue) == FALSE) {
			if (test_and_set_bit(WAKE_EVENT_XMIT, &pshare->xmit_wake) == 0)
				wake_up_process(pshare->xmit_thread);
		}
	}
	
	priv->pshare->nr_recvbuf_handled_in_irq = nr_recvbuf;
	priv->pshare->nr_xmitbuf_handled_in_irq += nr_xmitbuf;
	
	return err;
}

#else // !CONFIG_SDIO_TX_IN_INTERRUPT
s32 sd_rx_request_hdl(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	struct priv_shared_info *pshare = priv->pshare;
	
	struct recv_buf *precvbuf = NULL;
	s32 err = 0;
	int nr_recvbuf = 0;
	
	u32 buf[2];
	u8 *data = (u8*) buf;
	u32 sdio_hisr;
	
	do {
		if ((TRUE == pshare->bDriverStopped) || (TRUE == pshare->bSurpriseRemoved))
			break;
		
		err = _sdio_local_read(priv, SDIO_REG_HISR, 8, data);
		if (err)
			break;
		
		sdio_hisr = le32_to_cpu(*(u32*)data);
		if (sdio_hisr & SDIO_HISR_RX_REQUEST)
			pHalData->SdioRxFIFOSize = le16_to_cpu(*(u16*)&data[4]);

		if (pHalData->SdioRxFIFOSize)
		{
			precvbuf = sd_recv_rxfifo(priv, pHalData->SdioRxFIFOSize);
			
			if (precvbuf) {
				nr_recvbuf++;
				sd_rxhandler(priv, precvbuf);
			}
			
			pHalData->SdioRxFIFOSize = 0;
		} else {
			break;
		}
	} while (1);
	
	priv->pshare->nr_recvbuf_handled_in_irq = nr_recvbuf;
	
	return err;
}
#endif // CONFIG_SDIO_TX_IN_INTERRUPT

void sd_int_dpc(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	struct priv_shared_info *pshare = priv->pshare;

	if (pHalData->sdio_hisr & SDIO_HISR_CPWM1)
	{
		unsigned char state;

		_sdio_local_read(priv, SDIO_REG_HCPWM1, 1, &state);
	}

	if (pHalData->sdio_hisr & SDIO_HISR_TXERR)
	{
		u8 *status;
		u32 addr;

		status = _rtw_malloc(4);
		if (status)
		{
			addr = TXDMA_STATUS;
			HalSdioGetCmdAddr8723ASdio(priv, WLAN_IOREG_DEVICE_ID, addr, &addr);
			_sd_read(priv, addr, 4, status);
			_sd_write(priv, addr, 4, status);
			printk("%s: SDIO_HISR_TXERR (0x%08x)\n", __func__, le32_to_cpu(*(u32*)status));
			priv->pshare->tx_dma_err++;
			_rtw_mfree(status, 4);
		} else {
			printk("%s: SDIO_HISR_TXERR, but can't allocate memory to read status!\n", __func__);
		}
	}

#ifdef CONFIG_INTERRUPT_BASED_TXBCN

	if (pHalData->sdio_hisr & SDIO_HISR_BCNERLY_INT)
	{
		if (priv->timoffset)
			update_beacon(priv);
	}
	
	#ifdef  CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
	if (pHalData->sdio_hisr & (SDIO_HISR_TXBCNOK|SDIO_HISR_TXBCNERR))
	{
	}
	#endif
#endif //CONFIG_INTERRUPT_BASED_TXBCN

	if (pHalData->sdio_hisr & SDIO_HISR_C2HCMD)
	{
		printk("%s: C2H Command\n", __func__);
	}

#ifdef CONFIG_SDIO_TX_IN_INTERRUPT
	if (pHalData->sdio_hisr & (SDIO_HISR_AVAL|SDIO_HISR_RX_REQUEST))
	{
		// Handle unexpected TX interrupt
		if ((pHalData->sdio_hisr & SDIO_HISR_AVAL)
				&& !(pHalData->SdioTxIntStatus & BIT(SDIO_TX_INT_SETUP_TH)))
		{
			sdio_query_txbuf_status_locksafe(priv);
		}
	
		if (sd_tx_rx_hdl(priv) == -ENOMEDIUM) {
			pshare->bSurpriseRemoved = TRUE;
			return;
		}
	}

#else // !CONFIG_SDIO_TX_IN_INTERRUPT
#ifdef CONFIG_SDIO_TX_INTERRUPT
	if (pHalData->sdio_hisr & SDIO_HISR_AVAL)
	{
		if (test_and_clear_bit(SDIO_TX_INT_SETUP_TH, &pHalData->SdioTxIntStatus)) {
			// Invalidate TX Free Page Threshold
			RTL_W8(reg_freepage_thres[pHalData->SdioTxIntQIdx], 0xFF);
			
			sdio_query_txbuf_status(priv);
			
			if (test_and_set_bit(WAKE_EVENT_XMIT, &pshare->xmit_wake) == 0)
				wake_up_process(pshare->xmit_thread);
		}
	}
#endif
	
	if (pHalData->sdio_hisr & SDIO_HISR_RX_REQUEST)
	{
		pHalData->sdio_hisr ^= SDIO_HISR_RX_REQUEST;
		
		if (sd_rx_request_hdl(priv) == -ENOMEDIUM) {
			pshare->bSurpriseRemoved = TRUE;
			return;
		}
	}
#endif // CONFIG_SDIO_TX_IN_INTERRUPT
}

static void sd_sync_int_hdl(struct sdio_func *func)
{
	struct net_device *dev;
	struct rtl8192cd_priv *priv;
	struct priv_shared_info *pshare;
	HAL_INTF_DATA_TYPE *pHalData;
	u32 buf[1];
	u8 *data = (u8*) buf;
	
	dev = sdio_get_drvdata(func);
	// check if surprise removal occurs ? If yes, return right now.
	if (NULL == dev)
		return;
	
	priv = GET_DEV_PRIV(dev);
	pshare = priv->pshare;
	
	if ((TRUE == pshare->bDriverStopped) || (TRUE == pshare->bSurpriseRemoved)) {
		printk("[%s] disable SDIO interrupt due to driver stop\n", __func__);
		DisableSdioInterrupt(priv);
		return;
	}

#ifdef CONFIG_POWER_SAVE
	if (pshare->ap_ps_handle.suspend_processing) {
		printk("sd_sync_int_hdl suspend_processing return\n");
		DisableSdioInterrupt(priv);
		return;
	}
#endif

	++pshare->nr_interrupt;

	if (_sdio_local_read(priv, SDIO_REG_HISR, 4, data))
		return;

	pHalData = GET_HAL_INTF_DATA(priv);
	pHalData->sdio_hisr = le32_to_cpu(*(u32*)data);

	if (pHalData->sdio_hisr & pHalData->sdio_himr)
	{
		u32 v32;
		pHalData->sdio_hisr &= pHalData->sdio_himr;
		
		// Reduce the frequency of RX Request Interrupt during RX handling
		DisableSdioInterrupt(priv);

		// clear HISR
		v32 = pHalData->sdio_hisr & MASK_SDIO_HISR_CLEAR;
		if (v32) {
			v32 = cpu_to_le32(v32);
			_sdio_local_write(priv, SDIO_REG_HISR, 4, (u8*)&v32);
		}

		sd_int_dpc(priv);
		
		EnableSdioInterrupt(priv);
	}
	else
	{
		DEBUG_WARN("%s: HISR(0x%08x) and HIMR(0x%08x) not match!\n",
				__FUNCTION__, pHalData->sdio_hisr, pHalData->sdio_himr);
	}
}

int sdio_alloc_irq(struct rtl8192cd_priv *priv)
{
	struct sdio_func *func;
	int err;

	func = priv->pshare->psdio_func;
	
	sdio_claim_host(func);
	
	err = sdio_claim_irq(func, &sd_sync_int_hdl);
	
	sdio_release_host(func);
	
	if (err)
		printk(KERN_CRIT "%s: sdio_claim_irq FAIL(%d)!\n", __func__, err);
	
	return err;
}

int sdio_free_irq(struct rtl8192cd_priv *priv)
{
	struct sdio_func *func;
	int err;

	func = priv->pshare->psdio_func;
	
	sdio_claim_host(func);
	
	err = sdio_release_irq(func);
	
	sdio_release_host(func);
	
	if (err)
		printk(KERN_CRIT "%s: sdio_release_irq FAIL(%d)!\n", __func__, err);
	
	return err;
}

static int sdio_init(struct rtl8192cd_priv *priv)
{
	struct priv_shared_info *pshare = priv->pshare;
	struct sdio_func *func = pshare->psdio_func;
	int err;
	
	sdio_claim_host(func);

	err = sdio_enable_func(func);
	if (err) {
		printk(KERN_CRIT "%s: sdio_enable_func FAIL(%d)!\n", __func__, err);
		goto release;
	}

	err = sdio_set_block_size(func, 512);
	if (err) {
		printk(KERN_CRIT "%s: sdio_set_block_size FAIL(%d)!\n", __func__, err);
		goto release;
	}
	pshare->block_transfer_len = 512;
	pshare->tx_block_mode = 1;
	pshare->rx_block_mode = 1;

release:
	sdio_release_host(func);
	return err;
}

static void sdio_deinit(struct rtl8192cd_priv *priv)
{
	struct sdio_func *func;
	int err;

	func = priv->pshare->psdio_func;

	if (func) {
		sdio_claim_host(func);
		
		err = sdio_disable_func(func);
		if (err)
			printk(KERN_ERR "%s: sdio_disable_func(%d)\n", __func__, err);
/*
		err = sdio_release_irq(func);
		if (err)
			printk(KERN_ERR "%s: sdio_release_irq(%d)\n", __func__, err);
*/
		sdio_release_host(func);
	}
}

int sdio_dvobj_init(struct rtl8192cd_priv *priv)
{
	int err = 0;
	
	priv->pshare->pHalData = (HAL_INTF_DATA_TYPE *) rtw_zmalloc(sizeof(HAL_INTF_DATA_TYPE));
	if (NULL == priv->pshare->pHalData)
		return -ENOMEM;
	
	if ((err = sdio_init(priv)) != 0)
		goto fail;
	
	priv->pshare->version_id = VERSION_8188E;

	check_chipID_MIMO(priv);
	
	// family_mark: Move from xxx_init_one() to xxx_open() to let dynamically change OutPipe mapping
	//rtl8188es_interface_configure(priv);

	return 0;
	
fail:
	rtw_mfree(priv->pshare->pHalData, sizeof(HAL_INTF_DATA_TYPE));
	priv->pshare->pHalData = NULL;
	
	return err;
}

void sdio_dvobj_deinit(struct rtl8192cd_priv *priv)
{
	sdio_deinit(priv);

	if (priv->pshare->pHalData) {
		rtw_mfree(priv->pshare->pHalData, sizeof(HAL_INTF_DATA_TYPE));
		priv->pshare->pHalData = NULL;
	}
}

void rtw_dev_unload(struct rtl8192cd_priv *priv)
{
	struct priv_shared_info *pshare;
	
	pshare = priv->pshare;
	pshare->bDriverStopped = TRUE;

	if (FALSE == pshare->bSurpriseRemoved) {
		DisableSdioInterrupt(priv);
		sdio_free_irq(priv);
	}
	
#ifdef CHECK_HANGUP
	if (!priv->reset_hangup)
#endif
#ifdef SMART_REPEATER_MODE
	if (!pshare->switch_chan_rp)
#endif
	if (pshare->cmd_thread) {
		if (test_and_set_bit(WAKE_EVENT_CMD, &pshare->cmd_wake) == 0)
			wake_up_process(pshare->cmd_thread);
		printk("[%s] cmd_thread", __FUNCTION__);
		wait_for_completion(&pshare->cmd_thread_done);
		printk(" terminate\n");
		pshare->cmd_thread = NULL;
	}
	
	if (pshare->xmit_thread) {
		if (test_and_set_bit(WAKE_EVENT_XMIT, &pshare->xmit_wake) == 0)
			wake_up_process(pshare->xmit_thread);
		printk("[%s] xmit_thread", __FUNCTION__);
		wait_for_completion(&pshare->xmit_thread_done);
		printk(" terminate\n");
		pshare->xmit_thread = NULL;
	}

	if (FALSE == pshare->bSurpriseRemoved) {
		rtl8192cd_stop_hw(priv);
		pshare->bSurpriseRemoved = TRUE;
	}
}

#ifdef CONFIG_SDIO_RESERVE_MASSIVE_PUBLIC_PAGE
void _InitQueueReservedPage(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE	*pHalData	= GET_HAL_INTF_DATA(priv);
	
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ;
	u32			value32;
	u8			value8;

	switch (pHalData->Queue2Pipe[BE_QUEUE]) {
	case WLAN_TX_HIQ_DEVICE_ID:	numHQ = 9; break;
	case WLAN_TX_MIQ_DEVICE_ID:	numNQ = 9; break;
	case WLAN_TX_LOQ_DEVICE_ID:	numLQ = 9; break;
	}
	
	value8 = (u8)_NPQ(numNQ);
	RTL_W8(RQPN_NPQ, value8);

	numPubQ = TX_TOTAL_PAGE_NUMBER_88E - numHQ - numLQ - numNQ;

	// TX DMA
	value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;
	RTL_W32(RQPN, value32);
}
#else // !CONFIG_SDIO_RESERVE_MASSIVE_PUBLIC_PAGE
void _InitQueueReservedPage(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE	*pHalData	= GET_HAL_INTF_DATA(priv);
	
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ;
	u32			value32;
	u8			value8;
	
	BOOLEAN		bWiFiConfig	= ((priv->pmib->dot11OperationEntry.wifi_specific) ? TRUE : FALSE);
	
	if (bWiFiConfig)
	{
		if (pHalData->OutEpQueueSel & TX_SELE_HQ)
		{
			numHQ =  0x30;
		}

		if (pHalData->OutEpQueueSel & TX_SELE_LQ)
		{
			numLQ = 0x30;
		}

		// NOTE: This step shall be proceed before writting REG_RQPN.
		if (pHalData->OutEpQueueSel & TX_SELE_NQ) {
			numNQ = 0x30;
		}
		value8 = (u8)_NPQ(numNQ);
		RTL_W8(RQPN_NPQ, value8);

		numPubQ = TX_TOTAL_PAGE_NUMBER_88E - numHQ - numLQ - numNQ;

		// TX DMA
		value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;
		RTL_W32(RQPN, value32);
	}
	else
	{
		RTL_W16(RQPN_NPQ, 0x0000);
		RTL_W32(RQPN, 0x80a00900);
	}
}
#endif // CONFIG_SDIO_RESERVE_MASSIVE_PUBLIC_PAGE

static void _InitNormalChipRegPriority(struct rtl8192cd_priv *priv,
	u16 beQ, u16 bkQ, u16 viQ, u16 voQ, u16 mgtQ, u16 hiQ)
{
	u16 value16 = (RTL_R16(TRXDMA_CTRL) & 0x7);

	value16 |=	_TXDMA_BEQ_MAP(beQ) 	| _TXDMA_BKQ_MAP(bkQ) |
				_TXDMA_VIQ_MAP(viQ) 	| _TXDMA_VOQ_MAP(voQ) |
				_TXDMA_MGQ_MAP(mgtQ)| _TXDMA_HIQ_MAP(hiQ);
	
	RTL_W16(TRXDMA_CTRL, value16);
}

static void _InitNormalChipOneOutEpPriority(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	
	u16	value = 0;
	switch(pHalData->OutEpQueueSel)
	{
		case TX_SELE_HQ:
			value = QUEUE_HIGH;
			break;
		case TX_SELE_LQ:
			value = QUEUE_LOW;
			break;
		case TX_SELE_NQ:
			value = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	
	_InitNormalChipRegPriority(priv, value, value, value, value, value, value);

}

static void _InitNormalChipTwoOutEpPriority(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	u16	beQ,bkQ,viQ,voQ,mgtQ,hiQ;
	
	u16	valueHi = 0;
	u16	valueLow = 0;
	
	switch(pHalData->OutEpQueueSel)
	{
		case (TX_SELE_HQ | TX_SELE_LQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_NQ | TX_SELE_LQ):
			valueHi = QUEUE_NORMAL;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_HQ | TX_SELE_NQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}

	if(!priv->pmib->dot11OperationEntry.wifi_specific){
		beQ		= valueLow;
		bkQ		= valueLow;
		viQ		= valueHi;
		voQ		= valueHi;
		mgtQ	= valueHi; 
		hiQ		= valueHi;
	}
	else{//for WMM ,CONFIG_OUT_EP_WIFI_MODE
		beQ		= valueLow;
		bkQ		= valueHi;
		viQ		= valueHi;
		voQ		= valueLow;
		mgtQ	= valueHi;
		hiQ		= valueHi;
	}
	
	_InitNormalChipRegPriority(priv,beQ,bkQ,viQ,voQ,mgtQ,hiQ);

}

static void _InitNormalChipThreeOutEpPriority(struct rtl8192cd_priv *priv)
{
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;

	if(!priv->pmib->dot11OperationEntry.wifi_specific){// typical setting
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_LOW;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;
	}
	else{// for WMM
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_NORMAL;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;
	}
	
	_InitNormalChipRegPriority(priv,beQ,bkQ,viQ,voQ,mgtQ,hiQ);
}

static void _InitNormalChipQueuePriority(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);

	switch(pHalData->OutEpNumber)
	{
		case 1:
			_InitNormalChipOneOutEpPriority(priv);
			break;
		case 2:
			_InitNormalChipTwoOutEpPriority(priv);
			break;
		case 3:
			_InitNormalChipThreeOutEpPriority(priv);
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
}

void _InitQueuePriority(struct rtl8192cd_priv *priv)
{
	_InitNormalChipQueuePriority(priv);
}

static void HalRxAggr8188ESdio(struct rtl8192cd_priv *priv)
{
	u8	valueDMATimeout;
	u8	valueDMAPageCount;
	
	if (priv->pmib->dot11OperationEntry.wifi_specific)
	{
		// 2010.04.27 hpfan
		// Adjust RxAggrTimeout to close to zero disable RxAggr, suggested by designer
		// Timeout value is calculated by 34 / (2^n)
		valueDMATimeout = 0x0f;
		valueDMAPageCount = 0x01;
	}
	else
	{
		valueDMATimeout = 0x06;
		//valueDMAPageCount = 0x0F;
		//valueDMATimeout = 0x0a;
		valueDMAPageCount = 0x24;
	}
	
	RTL_W8(RXDMA_AGG_PG_TH+1, valueDMATimeout);
	RTL_W8(RXDMA_AGG_PG_TH, valueDMAPageCount);
}

void sdio_AggSettingRxUpdate(struct rtl8192cd_priv *priv)
{
	u8 valueDMA;
	
	valueDMA = RTL_R8(TRXDMA_CTRL);
	valueDMA |= RXDMA_AGG_EN;
	RTL_W8(TRXDMA_CTRL, valueDMA);
}

void _initSdioAggregationSetting(struct rtl8192cd_priv *priv)
{
	// Tx aggregation setting
	//sdio_AggSettingTxUpdate(priv);

	// Rx aggregation setting
	HalRxAggr8188ESdio(priv);
	sdio_AggSettingRxUpdate(priv);
}

//
//	Description:
//		Query SDIO Local register to get the current number of TX OQT Free Space.
//
u8 sdio_query_txoqt_status(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	pHalData->SdioTxOQTFreeSpace = SdioLocalCmd52Read1Byte(priv, SDIO_REG_OQT_FREE_SPACE);
	return TRUE;
}

//
//	Description:
//		Query SDIO Local register to get the current number of Free TxPacketBuffer page.
//
u8 sdio_query_txbuf_status(struct rtl8192cd_priv *priv)
{
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	u32 NumOfFreePage;

	NumOfFreePage = SdioLocalCmd53Read4Byte(priv, SDIO_REG_FREE_TXPG);
#ifdef _BIG_ENDIAN_
	NumOfFreePage = cpu_to_le32(NumOfFreePage);
#endif
	memcpy(pHalData->SdioTxFIFOFreePage, &NumOfFreePage, 4);

#if 0
	printk("%s: Free page for HIQ(%#x),MIQ(%#x),LOQ(%#x),PUBQ(%#x)\n",
			__FUNCTION__,
			pHalData->SdioTxFIFOFreePage[HI_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[MID_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[LOW_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[PUBLIC_QUEUE_IDX]);
#endif
	
	return TRUE;
}

#ifdef CONFIG_SDIO_TX_IN_INTERRUPT
u8 sdio_query_txbuf_status_locksafe(struct rtl8192cd_priv *priv)
{
	struct priv_shared_info *pshare = priv->pshare;
	HAL_INTF_DATA_TYPE *pHalData = GET_HAL_INTF_DATA(priv);
	unsigned long seq;
	u32 NumOfFreePage;
	
	_queue *pqueue = &pshare->pending_xmitbuf_queue;
	_irqL irql;

query:
	seq = pshare->freepage_updated_seq;
	smp_rmb();

	NumOfFreePage = SdioLocalCmd53Read4Byte(priv, SDIO_REG_FREE_TXPG);
#ifdef _BIG_ENDIAN_
	NumOfFreePage = cpu_to_le32(NumOfFreePage);
#endif
	
	xmit_lock(&pqueue->lock, &irql);
	if (!pshare->freepage_updated) {
		if (seq == pshare->freepage_updated_seq) {
		++pshare->freepage_updated_seq;
		memcpy(pHalData->SdioTxFIFOFreePage, &NumOfFreePage, SDIO_TX_FREE_PG_QUEUE);
		} else {
			// Avoid TX interrupt clear but free pages are not updated. If not query again, TX may stop.
			xmit_unlock(&pqueue->lock, &irql);
			goto query;
		}
	}
	xmit_unlock(&pqueue->lock, &irql);
	
#if 0
	printk("%s: Free page for HIQ(%#x),MIQ(%#x),LOQ(%#x),PUBQ(%#x)\n",
			__FUNCTION__,
			pHalData->SdioTxFIFOFreePage[HI_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[MID_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[LOW_QUEUE_IDX],
			pHalData->SdioTxFIFOFreePage[PUBLIC_QUEUE_IDX]);
#endif
	
	return TRUE;
}
#endif

static void pre_rtl8188es_beacon_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	struct priv_shared_info *pshare = priv->pshare;
	
	if ((pshare->bDriverStopped) || (pshare->bSurpriseRemoved)) {
		printk("[%s] bDriverStopped(%d) OR bSurpriseRemoved(%d)\n",
			__FUNCTION__, pshare->bDriverStopped, pshare->bSurpriseRemoved);
		return;
	}
	
	rtw_enqueue_timer_event(priv, &pshare->beacon_timer_event, ENQUEUE_TO_HEAD);
}

#define BEACON_EARLY_TIME		20	// unit:TU
static void rtl8188es_beacon_timer(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	struct priv_shared_info *pshare = priv->pshare;
	u32 beacon_interval;
	u32 timestamp[2];
	u64 time;
	u32 cur_tick, time_offset;
#ifdef MBSSID
	u32 inter_beacon_space;
	int nr_vap, idx, bcn_idx;
#endif
	u8 val8, reg_val8, late=0;
	
	beacon_interval = priv->pmib->dot11StationConfigEntry.dot11BeaconPeriod * NET80211_TU_TO_US;
	if (0 == beacon_interval) {
		printk("[%s] ERROR: beacon interval = 0\n", __FUNCTION__);
		return;
	}

#ifdef SDIO_AP_OFFLOAD
	if (pshare->offload_function_ctrl > RTW_PM_PREPROCESS) {
		mod_timer(&pshare->beacon_timer, jiffies+usecs_to_jiffies(beacon_interval));
		return;
	}
	
	if (pshare->offload_function_ctrl == RTW_PM_PREPROCESS && pshare->offload_prohibited) {
		pshare->offload_function_ctrl = RTW_PM_AWAKE;
	}
#endif

	timestamp[1] = RTL_R32(TSFTR+4);
	timestamp[0] = RTL_R32(TSFTR);
	while (timestamp[1]) {
		time = (u64)(0xFFFFFFFF % beacon_interval + 1) * timestamp[1] + timestamp[0];
		timestamp[0] = (u32)time;
		timestamp[1] = (u32)(time >> 32);
	}
	cur_tick = timestamp[0] % beacon_interval;
	
#ifdef MBSSID
	nr_vap = (pshare->nr_bcn > 1) ? (pshare->nr_bcn - 1) : 0;
	if (nr_vap) {
		inter_beacon_space = pshare->inter_bcn_space;//beacon_interval / (nr_vap+1);
		idx = cur_tick / inter_beacon_space;
		if (idx < nr_vap)	// if (idx < (nr_vap+1))
			bcn_idx = idx +1;	// bcn_idx = (idx + 1) % (nr_vap+1);
		else
			bcn_idx = 0;
		priv = pshare->bcn_priv[bcn_idx];
		if (((idx+2 == nr_vap+1) && (idx < nr_vap+1)) || (0 == bcn_idx)) {
			time_offset = beacon_interval - cur_tick - BEACON_EARLY_TIME * NET80211_TU_TO_US;
			if ((s32)time_offset < 0) {
				time_offset += inter_beacon_space;
			}
		} else {
			time_offset = (idx+2)*inter_beacon_space - cur_tick - BEACON_EARLY_TIME * NET80211_TU_TO_US;
			if (time_offset > (inter_beacon_space+(inter_beacon_space >> 1))) {
				time_offset -= inter_beacon_space;
				late = 1;
			}
		}
	} else
#endif // MBSSID
	{
		priv = pshare->bcn_priv[0];
		time_offset = 2*beacon_interval - cur_tick - BEACON_EARLY_TIME * NET80211_TU_TO_US;
		if (time_offset > (beacon_interval+(beacon_interval >> 1))) {
			time_offset -= beacon_interval;
			late = 1;
		}
	}
	
	BUG_ON((s32)time_offset < 0);
	
	mod_timer(&pshare->beacon_timer, jiffies+usecs_to_jiffies(time_offset));
	
#ifdef UNIVERSAL_REPEATER
	if (IS_ROOT_INTERFACE(priv)) {
		if ((OPMODE & WIFI_STATION_STATE) && GET_VXD_PRIV(priv) &&
				(GET_VXD_PRIV(priv)->drv_state & DRV_STATE_VXD_AP_STARTED)) {
			priv = GET_VXD_PRIV(priv);
		}
	}
#endif
	
	if (late)
		++priv->ext_stats.beacon_er;
	
	if (priv->timoffset) {
		update_beacon(priv);
		
		// handle any buffered BC/MC frames
		reg_val8 = RTL_R8(BCN_CTRL);
		val8 = *((unsigned char *)priv->beaconbuf + priv->timoffset + 4);
		
		if (val8 & 0x01) {
			if(reg_val8 & DIS_ATIM)
				RTL_W8(BCN_CTRL, (reg_val8 & (~DIS_ATIM)));
			process_mcast_dzqueue(priv);
			priv->pkt_in_dtimQ = 0;
		} else {
			if(!(reg_val8 & DIS_ATIM))
				RTL_W8(BCN_CTRL, (reg_val8 | DIS_ATIM));
		}
	}
}

u8 rtw_init_drv_sw(struct rtl8192cd_priv *priv)
{
	if (_rtw_init_cmd_priv(priv) == FAIL)
		goto cmd_fail;
	
	if (_rtw_init_xmit_priv(priv) == FAIL)
		goto xmit_fail;

	if (_rtw_init_recv_priv(priv) == FAIL)
		goto recv_fail;
	
	init_timer(&priv->pshare->beacon_timer);
	priv->pshare->beacon_timer.data = (unsigned long)priv;
	priv->pshare->beacon_timer.function = pre_rtl8188es_beacon_timer;
	INIT_TIMER_EVENT_ENTRY(&priv->pshare->beacon_timer_event,
		rtl8188es_beacon_timer, (unsigned long)priv);
	
#ifdef SDIO_AP_OFFLOAD
	_rtw_spinlock_init(&priv->pshare->offload_lock);
#ifdef CONFIG_POWER_SAVE
	_rtw_mutex_init(&priv->pshare->apps_lock);
#endif
#endif

	return SUCCESS;

recv_fail:
	_rtw_free_xmit_priv(priv);
xmit_fail:
	_rtw_free_cmd_priv(priv);
cmd_fail:
	
	return FAIL;
}

u8 rtw_free_drv_sw(struct rtl8192cd_priv *priv)
{
	_rtw_free_recv_priv(priv);
	_rtw_free_xmit_priv(priv);
	_rtw_free_cmd_priv(priv);
	
#ifdef SDIO_AP_OFFLOAD
	_rtw_spinlock_free(&priv->pshare->offload_lock);
#ifdef CONFIG_POWER_SAVE
	_rtw_mutex_free(&priv->pshare->apps_lock);
#endif
#endif

	return SUCCESS;
}

