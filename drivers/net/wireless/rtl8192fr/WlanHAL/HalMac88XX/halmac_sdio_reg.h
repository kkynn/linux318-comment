#ifndef __HALMAC_SDIO_REG_H__
#define __HALMAC_SDIO_REG_H__

/* SDIO CMD address mapping */

#define HALMAC_SDIO_4BYTE_LEN_MASK      0x1FFF
#define HALMAC_SDIO_LOCAL_MSK           0x0FFF
#define HALMAC_WLAN_MAC_REG_MSK			0xFFFF
#define HALMAC_WLAN_IOREG_MSK           0xFFFF

/* Sdio Address for SDIO Local Reg, TRX FIFO, MAC Reg */
typedef enum {
	HALMAC_SDIO_CMD_ADDR_SDIO_REG = 0,
	HALMAC_SDIO_CMD_ADDR_MAC_REG = 8,
	HALMAC_SDIO_CMD_ADDR_TXFF_HIGH = 4,
	HALMAC_SDIO_CMD_ADDR_TXFF_LOW = 6,
	HALMAC_SDIO_CMD_ADDR_TXFF_NORMAL = 5,
	HALMAC_SDIO_CMD_ADDR_TXFF_EXTRA = 7,
	HALMAC_SDIO_CMD_ADDR_RXFF = 7,
} HALMAC_SDIO_CMD_ADDR;

/* IO Bus domain address mapping */
#define SDIO_LOCAL_OFFSET                                       0x10250000
#define WLAN_IOREG_OFFSET                               0x10260000
#define FW_FIFO_OFFSET                                  0x10270000
#define TX_HIQ_OFFSET                                           0x10310000
#define TX_MIQ_OFFSET                                           0x10320000
#define TX_LOQ_OFFSET                                           0x10330000
#define TX_EXQ_OFFSET                                           0x10350000
#define RX_RXOFF_OFFSET                                 0x10340000

/* Get TX WLAN FIFO information in CMD53 addr  */
#if (HALMAC_8822B_SUPPORT || HALMAC_8821C_SUPPORT)
#define GET_WLAN_TXFF_DEVICE_ID(__pCmd53_addr)		LE_BITS_TO_4BYTE((u32 *)__pCmd53_addr, 13, 4)
#define GET_WLAN_TXFF_PKT_SIZE(__pCmd53_addr)		(LE_BITS_TO_4BYTE((u32 *)__pCmd53_addr, 0, 13) << 2)
#endif

#endif/* __HALMAC_SDIO_REG_H__ */
