#include "../halmac_88xx_cfg.h"
#include "halmac_8821c_cfg.h"

#define CLKCAL_CTRL_PHYPARA		0x00
#define CLKCAL_SET_PHYPARA		0x20
#define CLKCAL_TRG_VAL_PHYPARA	0x21
#define PCIE_L1_BACKDOOR		0x719

static HALMAC_RET_STATUS
halmac_auto_refclk_cal_8821c_pcie(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_INTGRA pIntf_intgra
);

/**
 * halmac_mac_power_switch_8821c_pcie() - switch mac power
 * @pHalmac_adapter : the adapter of halmac
 * @halmac_power : power state
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mac_power_switch_8821c_pcie(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_MAC_POWER	halmac_power
)
{
	u8 interface_mask;
	u8 value8;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	halmac_api_record_id_88xx(pHalmac_adapter, HALMAC_API_MAC_POWER_SWITCH);

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_mac_power_switch_88xx_pcie halmac_power =  %x ==========>\n", halmac_power);
	interface_mask = HALMAC_PWR_INTF_PCI_MSK;

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_CR);
	if (value8 == 0xEA)
		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_OFF;
	else
		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_ON;

	/* Check if power switch is needed */
	if (halmac_power == HALMAC_MAC_POWER_ON && pHalmac_adapter->halmac_state.mac_power == HALMAC_MAC_POWER_ON) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_WARN, "halmac_mac_power_switch power state unchange!\n");
		return HALMAC_RET_PWR_UNCHANGE;
	}

	if (halmac_power == HALMAC_MAC_POWER_OFF) {
		if (halmac_pwr_seq_parser_88xx(pHalmac_adapter, HALMAC_PWR_CUT_ALL_MSK, HALMAC_PWR_FAB_TSMC_MSK,
			    interface_mask, halmac_8821c_card_disable_flow) != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "Handle power off cmd error\n");
			return HALMAC_RET_POWER_OFF_FAIL;
		}

		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_OFF;
		pHalmac_adapter->halmac_state.ps_state = HALMAC_PS_STATE_UNDEFINE;
		pHalmac_adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;
		halmac_init_adapter_dynamic_para_88xx(pHalmac_adapter);
	} else {
		if (halmac_pwr_seq_parser_88xx(pHalmac_adapter, HALMAC_PWR_CUT_ALL_MSK, HALMAC_PWR_FAB_TSMC_MSK,
			    interface_mask, halmac_8821c_card_enable_flow) != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_ERR, "Handle power on cmd error\n");
			return HALMAC_RET_POWER_ON_FAIL;
		}

		pHalmac_adapter->halmac_state.mac_power = HALMAC_MAC_POWER_ON;
		pHalmac_adapter->halmac_state.ps_state = HALMAC_PS_STATE_ACT;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_mac_power_switch_88xx_pcie <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pcie_switch_8821c() - pcie gen1/gen2 switch
 * @pHalmac_adapter : the adapter of halmac
 * @pcie_cfg : gen1/gen2 selection
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pcie_switch_8821c(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PCIE_CFG	pcie_cfg
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	halmac_api_record_id_88xx(pHalmac_adapter, HALMAC_API_PCIE_SWITCH);

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_pcie_switch_8821c ==========>\n");


	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_pcie_switch_8821c <==========\n");

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_pcie_switch_8821c_nc(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_PCIE_CFG	pcie_cfg
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	halmac_api_record_id_88xx(pHalmac_adapter, HALMAC_API_PCIE_SWITCH);

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_pcie_switch_8821c_nc ==========>\n");

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_pcie_switch_8821c_nc <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_phy_cfg_8821c_pcie() - phy config
 * @pHalmac_adapter : the adapter of halmac
 * Author : KaiYuan Chang / Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_phy_cfg_8821c_pcie(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_INTF_PHY_PLATFORM platform
)
{
	VOID *pDriver_adapter = NULL;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	halmac_api_record_id_88xx(pHalmac_adapter, HALMAC_API_PHY_CFG);

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_phy_cfg ==========>\n");

	status = halmac_parse_intf_phy_88xx(pHalmac_adapter, HALMAC_RTL8821C_PCIE_PHY_GEN1, platform, HAL_INTF_PHY_PCIE_GEN1);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = halmac_parse_intf_phy_88xx(pHalmac_adapter, HALMAC_RTL8821C_PCIE_PHY_GEN2, platform, HAL_INTF_PHY_PCIE_GEN2);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_PWR, HALMAC_DBG_TRACE, "halmac_phy_cfg <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_interface_integration_tuning_8821c_pcie() - pcie interface fine tuning
 * @pHalmac_adapter : the adapter of halmac
 * Author : Rick Liu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_interface_integration_tuning_8821c_pcie(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_INTGRA pIntf_intgra
)
{
	HALMAC_RET_STATUS status;

	status = halmac_auto_refclk_cal_8821c_pcie(pHalmac_adapter, pIntf_intgra);

	return status;
}

static HALMAC_RET_STATUS
halmac_auto_refclk_cal_8821c_pcie(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_INTF_INTGRA pIntf_intgra
)
{
	u8 pcie_l1_backdoor_ori;
	u16 tmp_u16;
	u16 div_u16;
	u16 margin_u16;
	u16 cal_target;
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	VOID *pDriver_adapter = NULL;
	BOOLEAN l1_write_flag = false;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	/* Disable L1 backdoor at 0x719[4:3] */
	pcie_l1_backdoor_ori = halmac_dbi_read8_88xx(pHalmac_adapter, PCIE_L1_BACKDOOR);
	if (pcie_l1_backdoor_ori & (BIT(4) | BIT(3))) {
		status = halmac_dbi_write8_88xx(pHalmac_adapter, PCIE_L1_BACKDOOR, pcie_l1_backdoor_ori & ~(BIT(4) | BIT(3)));
		if (status != HALMAC_RET_SUCCESS)
			return status;
		l1_write_flag = true;
	}

	/* Disable this function before configuration*/
	tmp_u16 = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	if (tmp_u16 & BIT(9)) {
		status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, tmp_u16 & ~(BIT(9)), HAL_INTF_PHY_PCIE_GEN1);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}

	/* Check div and margin value, if div or margin is 0x?0, then disable this function */
	if ((0x00 == (pIntf_intgra->refclk_cal_div & 0x0F)) | (0x00 == (pIntf_intgra->refclk_cal_margin & 0x0F))) {
		if (l1_write_flag)
			status = halmac_dbi_write8_88xx(pHalmac_adapter, PCIE_L1_BACKDOOR, pcie_l1_backdoor_ori);
		return status;
	}

	/* Set reference clock div number at 0x00[7:6] */
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_auto_refclk_cal_8821c_pcie ==========>\n");
	div_u16 = 0x0003 & (pIntf_intgra->refclk_cal_div - 1);
	tmp_u16 = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, tmp_u16 & ~(BIT(7) | BIT(6)) | (div_u16 << 6), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	/* Set 0x00[11]=1 to count 1T of reference clock and read target value at 0x21[11:0] */
	tmp_u16 = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, tmp_u16 | BIT(11), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;
	PLATFORM_RTL_DELAY_US(pDriver_adapter, 22);
	cal_target = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_TRG_VAL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	if (!cal_target)
		return HALMAC_RET_FAIL;
	tmp_u16 = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, tmp_u16 & ~(BIT(11)), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	/* Set calibration target at 0x20[11:0] and margin at 0x20[15:12] */
	margin_u16 = 0x000F & pIntf_intgra->refclk_cal_margin;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]calib target = 0x%X, div = 0x%X, margin = 0x%X\n", cal_target, div_u16, margin_u16);
	status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_SET_PHYPARA, cal_target & 0x0FFF | (margin_u16 << 12), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	/* Turn on calibration mechanium at 0x00[9] */
	tmp_u16 = halmac_mdio_read_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, HAL_INTF_PHY_PCIE_GEN1);
	status = halmac_mdio_write_88xx(pHalmac_adapter, CLKCAL_CTRL_PHYPARA, tmp_u16 | BIT(9), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]halmac_auto_refclk_cal_8821c_pcie <==========\n");

	/* Set L1 backdoor to ori value at 0x719[4:3] */
	if (l1_write_flag)
		status = halmac_dbi_write8_88xx(pHalmac_adapter, PCIE_L1_BACKDOOR, pcie_l1_backdoor_ori);

	return status;
}
