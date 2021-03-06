 /*
  * Copyright (C) 2012 Realtek Semiconductor Corp.
  * All Rights Reserved.
  *
  * This program is the proprietary software of Realtek Semiconductor
  * Corporation and/or its licensors, and only be used, duplicated,
  * modified or distributed under the authorized license from Realtek.
  *
  * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
  * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 */


/*
 * Include Files
 */

#include <rtk/switch.h>
#include <rtk_rg_rtl9603d_asicDriver.h>
#if defined(CONFIG_APOLLOPRO_FPGA)
#include "rtk_rg_apolloPro_fpga.h"
#include <rtk_rg_internal.h>

#elif defined(CONFIG_RTK_L34_XPON_PLATFORM)
#include <rtk_rg_fc_internal.h>
#include <rtk_rg_fc_debug.h>
#else // RG romedriver
#include <rtk_rg_internal.h>
#include <ioal/mem32.h>
#endif
#include <rtk_rg_apolloPro_internal.h>
#include <dal/rtl9603d/dal_rtl9603d.h>


/*
 * Symbol Definition
 */


#define RTL9603D_L34_ASIC_TABLE_WORD 8
#define RTL9603D_FLOW_TCAM_DATABIT 0x8000

#define RETRY_TO_LONG	1000		// long timeout
#define RETRY_TO_SHORT	100			// short timeout

/*
 * Data Declaration
 */
static uint32 fb_init = {INIT_NOT_COMPLETED};
static uint32 fb_cc_init = {INIT_NOT_COMPLETED};

/* supported revision: CHIP_REV_ID_A(0x1), CHIP_REV_ID_B(0x2) */
uint8 ASICDRIVERVER = 0x1;

/*L34 table entry*/
MEM_ENTRY_DECLARE(rtl9603d_asic_hsb_entry_t, 13);
MEM_ENTRY_DECLARE(rtl9603d_asic_hsa_entry_t, 7);
MEM_ENTRY_DECLARE(rtl9603d_asic_netif_entry_t, 5);
MEM_ENTRY_DECLARE(rtl9603d_asic_flow_entry_t, 8);
MEM_ENTRY_DECLARE(rtl9603d_asic_etherType_entry_t, 1);
MEM_ENTRY_DECLARE(rtl9603d_asic_extPortMask_entry_t, 1);
MEM_ENTRY_DECLARE(rtl9603d_asic_indMac_entry_t, 1);
MEM_ENTRY_DECLARE(rtl9603d_asic_extraTag_entry_t, 1);

/*
 * Macro Definition
 */


/* Function Name:
 *      table_field_get
 * Description:
 *      Get the value from one specified field of table in buffer.
 * Input:
 *      table  - table index
 *      field  - field index of the table
 *      pData  - pointer buffer of table entry data
 * Output:
 *      pValue - pointer buffer of value from the specified field of table
 * Return:
 *      RT_ERR_OK                 - OK
 *      RT_ERR_FAILED             - Failed
 *      RT_ERR_OUT_OF_RANGE       - input parameter out of range
 *      RT_ERR_NULL_POINTER       - input parameter is null pointer
 *      RT_ERR_CHIP_NOT_SUPPORTED - functions not supported by this chip model
 * Note:
 *      1. The API is used when *value argument is WORD type.
 */
static int32
rg_table_field_get(
    uint32  table,
    uint32  field,
    uint32  *pValue,
    uint32  *pData)
{
    int32               field_bit_pos, data_word_pos, data_bit_pos;
    int32               i, unprocess_len;
    rtk_table_t         *pTable = NULL;
    rtk_tableField_t    *pTblField = NULL;

    RT_LOG(LOG_TRACE, MOD_HAL, "table=%d, field=%d", table, field);

    /* parameter check */
    RT_PARAM_CHK((table >= HAL_GET_MAX_TABLE_IDX()), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK(((NULL == pValue) || (NULL == pData)), RT_ERR_NULL_POINTER);
    pTable = table_find(table);
    /* NULL means the table is not supported in this chip*/
    RT_PARAM_CHK((NULL == pTable), RT_ERR_CHIP_NOT_SUPPORTED);
    RT_PARAM_CHK((field >= pTable->field_num), RT_ERR_OUT_OF_RANGE);

    pTblField = &pTable->fields[field];

    /* Base on pTblField->lsp and pTblField->len to process */
    field_bit_pos = pTblField->lsp;

    /* Caculate the data LSB bit */
    data_word_pos = field_bit_pos >> 5;
    data_bit_pos = field_bit_pos & 31;
    i = 0;

    /* Process single bit request */
    if (1 == pTblField->len)
    {
        if (pData[data_word_pos] & (1 << data_bit_pos))
            pValue[0] = 1;
        else
            pValue[0] = 0;
        return RT_ERR_OK;
    }

    /* Process multiple bits request - can process more than 32-bits in one field  */
    for (unprocess_len = pTblField->len; unprocess_len > 0; unprocess_len -= 32, i++)
    {
        if (data_bit_pos)
        {
            pValue[i] = (pData[data_word_pos] >> data_bit_pos) & ((1 << (32 - data_bit_pos)) - 1);
            data_word_pos++;
            pValue[i] |= (pData[data_word_pos] << (32 - data_bit_pos));
        }
        else
        {
            pValue[i] = pData[data_word_pos];
            data_word_pos++;
        }

        if (unprocess_len < 32)
        {
            pValue[i] &= ((1 << unprocess_len) - 1);
        }
    }

    return RT_ERR_OK;
} /* end of table_field_get */


/* Function Name:
 *      table_field_set
 * Description:
 *      Set the value to one specified field of table in buffer.
 * Input:
 *      table  - table index
 *      field  - field index of the table
 *      pValue - pointer buffer of value from the specified field of table
 * Output:
 *      pData  - pointer buffer of table entry data
 * Return:
 *      RT_ERR_OK                 - OK
 *      RT_ERR_FAILED             - Failed
 *      RT_ERR_OUT_OF_RANGE       - input parameter out of range
 *      RT_ERR_NULL_POINTER       - input parameter is null pointer
 *      RT_ERR_CHIP_NOT_SUPPORTED - functions not supported by this chip model
 * Note:
 *      1. The API is used when *pValue argument is WORD type.
 */
static int32
rg_table_field_set(
    uint32  table,
    uint32  field,
    uint32  *pValue,
    uint32  *pData)
{
    uint32              masks;
    int32               field_bit_pos, data_word_pos, data_bit_pos;
    int32               i, unprocess_len;
    rtk_table_t         *pTable = NULL;
    rtk_tableField_t    *pTblField = NULL;

    RT_LOG(LOG_TRACE, MOD_HAL, "table=%d, field=%d", table, field);

    /* parameter check */
    RT_PARAM_CHK((table >= HAL_GET_MAX_TABLE_IDX()), RT_ERR_OUT_OF_RANGE);
    RT_PARAM_CHK((NULL == pValue), RT_ERR_NULL_POINTER);
    RT_PARAM_CHK((NULL == pData), RT_ERR_NULL_POINTER);
    pTable = table_find(table);
    /* NULL means the table is not supported in this chip  */
    RT_PARAM_CHK((NULL == pTable), RT_ERR_CHIP_NOT_SUPPORTED);
    RT_PARAM_CHK((field >= pTable->field_num), RT_ERR_OUT_OF_RANGE);

    pTblField = &pTable->fields[field];

    /* Base on pTblField->lsp and pTblField->len to process */
    field_bit_pos = pTblField->lsp;

    /* Caculate the data LSB bit */
    data_word_pos = field_bit_pos >> 5;
    data_bit_pos = field_bit_pos & 31;
    i = 0;

    /* Process multiple bits request - can process more than 32-bits in one field  */
    for (unprocess_len = pTblField->len; unprocess_len > 0; unprocess_len -= 32, i++)
    {
        if (data_bit_pos)
        {
            if (unprocess_len >= 32)
            {
                masks = -1;
            }
            else
            {
                masks = (1 << unprocess_len) - 1;
                pValue[i] &= masks;
            }

            pData[data_word_pos] &= ~(masks << data_bit_pos);
            pData[data_word_pos] |= (pValue[i] << data_bit_pos);
            data_word_pos++;
            pData[data_word_pos] &= ~(masks >> (32 - data_bit_pos));
            pData[data_word_pos] |= (pValue[i] >> (32 - data_bit_pos)) & ((1 << data_bit_pos) - 1);
        }
        else
        {
            if (unprocess_len >= 32)
            {
                pData[data_word_pos] = pValue[i];
            }
            else
            {
                masks = (1 << unprocess_len) - 1;
                pValue[i] &= masks;
                pData[data_word_pos] &= ~masks;
                pData[data_word_pos] |= pValue[i];
            }
            data_word_pos++;
        }
    }

    return RT_ERR_OK;
} /* end of table_field_set */

/* Function Name:
 *      table_field_mac_get
 * Description:
 *      Get the mac address from one specified field of table in buffer.
 * Input:
 *      table  - table index
 *      field  - field index of the table
 *      pData  - pointer buffer of table entry data
 * Output:
 *      pValue - pointer buffer of value from the specified field of table
 * Return:
 *      RT_ERR_OK     - OK
 *      RT_ERR_FAILED - Failed
 * Note:
 *      1. The API is used for mac address type only
 */
static int32
rg_table_field_mac_get(
    uint32  table,
    uint32  field,
    uint8   *pValue,
    uint32  *pData)
{
    uint32  ret = RT_ERR_FAILED;
    uint32  temp_mac[2];

    if ((ret = rg_table_field_get(table, field, &temp_mac[0], pData)) != RT_ERR_OK)
        return ret;
	
    *(pValue + 5) = (uint8) (temp_mac[0] & 0x000000FF);
    *(pValue + 4) = (uint8) ((temp_mac[0] & 0x0000FF00) >> 8);
    *(pValue + 3) = (uint8) ((temp_mac[0] & 0x00FF0000) >> 16);
    *(pValue + 2) = (uint8) ((temp_mac[0] & 0xFF000000) >> 24);
    *(pValue + 1) = (uint8) (temp_mac[1] & 0x000000FF);
    *(pValue + 0) = (uint8) ((temp_mac[1] & 0x0000FF00) >> 8);

    return RT_ERR_OK;
} /* end of table_field_mac_get */


/* Function Name:
 *      table_field_mac_set
 * Description:
 *      Set the mac address to one specified field of table in buffer.
 * Input:
 *      table  - table index
 *      field  - field index of the table
 *      pValue - pointer buffer of value from the specified field of table
 * Output:
 *      pData  - pointer buffer of table entry data
 * Return:
 *      RT_ERR_OK     - OK
 *      RT_ERR_FAILED - Failed
 * Note:
 *      1. The API is used for mac address type only
 */
static int32
rg_table_field_mac_set(
    uint32  table,
    uint32  field,
    uint8   *pValue,
    uint32  *pData)
{
    uint32  ret = RT_ERR_FAILED;
    uint32  temp_mac[2];

    temp_mac[0] = (((uint32) *(pValue + 2)) << 24) | (((uint32) *(pValue + 3)) << 16) |
                   (((uint32) *(pValue + 4)) << 8) | ((uint32) *(pValue + 5));
    temp_mac[1] = (((uint32) *(pValue + 0)) << 8) | ((uint32) *(pValue + 1));


    if ((ret = rg_table_field_set(table, field, temp_mac, pData)) != RT_ERR_OK)
        return ret;

    return RT_ERR_OK;
} /* end of table_field_mac_set */

static rtk_rg_err_code_t rg_asic_table_read(
    uint32  table,
    uint32  addr,
    uint32  *pData)
{
	uint32      reg_data,field_data;
	uint32      busy, retry;
	uint32      i;
	int32       ret = RT_ERR_FAILED;
	uint32      l34_table_data[RTL9603D_L34_ASIC_TABLE_WORD];

	rtk_table_t *pTable = NULL;
	RT_DBG(LOG_DEBUG, (MOD_HAL), "rtl9607c_table_read table=%d, addr=0x%x", table, addr);
	/* parameter check */
	RT_PARAM_CHK((table >= HAL_GET_MAX_TABLE_IDX()), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((NULL == pData), RT_ERR_RG_NULL_POINTER);

	pTable = table_find(table);
	/* NULL means the table is not supported in this chip unit */
	RT_PARAM_CHK((NULL == pTable), RT_ERR_RG_CHIP_NOT_SUPPORT);	
	if(table==RTL9603D_TCAMt)
		RT_PARAM_CHK((addr >= ((pTable->size) + RTL9603D_FLOW_TCAM_DATABIT)), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	else
		RT_PARAM_CHK((addr >= pTable->size), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	switch(table)
	{
	    case RTL9603D_CAMt:
	    case RTL9603D_CAM_TAGt:
	    case RTL9603D_ETHER_TYPEt:
	    case RTL9603D_FB_EXT_PORTt:
	    case RTL9603D_FLOW_TABLE_PATH1_2t:
	    case RTL9603D_FLOW_TABLE_PATH3_4t:
	    case RTL9603D_FLOW_TABLE_PATH5t:
	    case RTL9603D_FLOW_TABLE_PATH6t:
	    case RTL9603D_FLOW_TABLE_TAGt:
	    case RTL9603D_INTERFACEt:
	    case RTL9603D_MAC_IDXt:
	    case RTL9603D_TCAMt:	
			break;
	
	    default:
	        return RT_ERR_RG_FAILED;
	        break;
	}

	/* initialize variable */
	reg_data = 0;
	busy = 0;

	//ASIC("prepare to read table: %d, ind type: %d, entry: %d", table, pTable->type, addr);

	osal_memset(&l34_table_data, 0, sizeof(l34_table_data));

	/* Table access operation
	 */
	field_data = 1;
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_RD_EXEf, &field_data, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* access table type */
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_TBL_IDXf, (uint32 *)&(pTable->type), &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Select access address of the table */
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_ETRY_IDXf, &addr, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Write indirect control register to start the read operation */
	if ((ret = reg_write(RTL9603D_NAT_TBL_ACCESS_CTRLr, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Wait operation completed */
	retry = 0;
	do
	{
	    if ((ret = reg_field_read(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_RD_EXEf, &busy)) != RT_ERR_OK)
	    {
	        return ret;
	    }
		retry++;
	} while((1 == busy) && (retry < RETRY_TO_LONG));

	if(retry >= RETRY_TO_LONG)
		return RT_ERR_BUSYWAIT_TIMEOUT;

	/* Read table data from indirect data register */
	for (i = 0 ; i < RTL9603D_L34_ASIC_TABLE_WORD ; i++)
	{
	    if ((ret = reg_array_read(RTL9603D_NAT_TBL_ACCESS_RDDATAr, REG_ARRAY_INDEX_NONE, i, &l34_table_data[i])) != RT_ERR_OK)
	    {
	        return ret;
	    }
	}

	for (i = 0 ; i < RTL9603D_L34_ASIC_TABLE_WORD ; i++)
	{
		if(i >= pTable->datareg_num)
			break;

		pData[i]= l34_table_data[i];
		//ASIC("read data[%d] 0x%08x", i, l34_table_data[i]);
	}

	return RT_ERR_RG_OK;
}

static rtk_rg_err_code_t rg_asic_table_write(
    uint32  table,
    uint32  addr,
    uint32  *pData)
{
	uint32      reg_data,field_data;
	uint32      busy, retry;
	uint32      i;
	int32       ret = RT_ERR_FAILED;
	uint32      l34_table_data[RTL9603D_L34_ASIC_TABLE_WORD];
	
	rtk_table_t *pTable = NULL;

	/* parameter check */
	RT_PARAM_CHK((table >= HAL_GET_MAX_TABLE_IDX()), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	RT_PARAM_CHK((NULL == pData), RT_ERR_RG_NULL_POINTER);
	pTable = table_find(table);
	/* NULL means the table is not supported in this chip unit */
	RT_PARAM_CHK((NULL == pTable), RT_ERR_RG_CHIP_NOT_SUPPORT);
	if(table==RTL9603D_TCAMt)
		RT_PARAM_CHK((addr >= ((pTable->size) + RTL9603D_FLOW_TCAM_DATABIT)), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	else
		RT_PARAM_CHK((addr >= pTable->size), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	switch(table)
	{
	    case RTL9603D_CAMt:
	    case RTL9603D_CAM_TAGt:
	    case RTL9603D_ETHER_TYPEt:
	    case RTL9603D_FB_EXT_PORTt:
	    case RTL9603D_FLOW_TABLE_PATH1_2t:
	    case RTL9603D_FLOW_TABLE_PATH3_4t:
	    case RTL9603D_FLOW_TABLE_PATH5t:
	    case RTL9603D_FLOW_TABLE_PATH6t:
	    case RTL9603D_FLOW_TABLE_TAGt:
	    case RTL9603D_INTERFACEt:
	    case RTL9603D_MAC_IDXt:
	    case RTL9603D_TCAMt:			
			break;
	
	    default:
	        return RT_ERR_RG_FAILED;
	        break;
	}

	//ASIC("prepare to write table: %d, ind type: %d, extry: %d", table, pTable->type, addr);

	/* initialize variable */
	reg_data = 0;
	busy = 0;
	osal_memset(&l34_table_data, 0, sizeof(l34_table_data));

	for (i = 0 ; i < RTL9603D_L34_ASIC_TABLE_WORD ; i++)
	{
		if(i >= pTable->datareg_num)
		    break;
		
		l34_table_data[i] = pData[i];
		//ASIC("write data[%d] 0x%08x", i, l34_table_data[i]);
	}

	/* Write pre-configure table data to indirect data register */
	for (i = 0 ; i < RTL9603D_L34_ASIC_TABLE_WORD ; i++)
	{
	    if ((ret = reg_array_write(RTL9603D_NAT_TBL_ACCESS_WRDATAr, REG_ARRAY_INDEX_NONE, i, &l34_table_data[i])) != RT_ERR_OK)
	    {
	        return ret;
	    }
	}

	/* Table access operation
	 */
	field_data =1;
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_WR_EXEf, &field_data, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* access table type */
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_TBL_IDXf, (uint32 *)&(pTable->type), &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Select access address of the table */
	if ((ret = reg_field_set(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_ETRY_IDXf, &addr, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Write indirect control register to start the write operation */
	if ((ret = reg_write(RTL9603D_NAT_TBL_ACCESS_CTRLr, &reg_data)) != RT_ERR_OK)
	{
	    return ret;
	}

	/* Wait operation completed */
	retry = 0;
	do
	{
	    if ((ret = reg_field_read(RTL9603D_NAT_TBL_ACCESS_CTRLr, RTL9603D_WR_EXEf, &busy)) != RT_ERR_OK)
	    {
	        return ret;
	    }
	    retry++;
	} while((busy) && (retry < RETRY_TO_LONG));

	if(retry >= RETRY_TO_LONG)
		return RT_ERR_BUSYWAIT_TIMEOUT;  

	return RT_ERR_RG_OK;
} /* end of table_write */


rtk_rg_err_code_t rtk_rg_asic_l2ArpUsageAsKnown_set(rtk_enable_t state)
{
	// LUT_L34_ARP_USAGE_AS_KNOWN
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal = 0;
		
	/* parameter check */
	RT_PARAM_CHK((RTK_ENABLE_END <= state), RT_ERR_RG_INVALID_PARAM);

	tmpVal = state;
	
	if ((ret = reg_field_write(RTL9603D_LUT_CFGr, RTL9603D_LUT_L34_ARP_USAGE_AS_KNOWNf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_l2LookupMIssAct_set(rtk_rg_asic_forwardAction_t action)
{
	// L34_L2_LOOKUP_MISS_ACT
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal = 0;
		
	/* parameter check */
	RT_PARAM_CHK((FB_ACTION_END <= action), RT_ERR_RG_INVALID_PARAM);


	if(action == FB_ACTION_DROP)
		tmpVal = 0;
	else if (action == FB_ACTION_TRAP2CPU)
		tmpVal = 1;
	else
		return RT_ERR_RG_INVALID_PARAM;
	
	if ((ret = reg_field_write(RTL9603D_LUT_CFGr, RTL9603D_L34_L2_LOOKUP_MISS_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_l2tpUdpSport_set(uint32 sport)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	
	/* parameter check */
	RT_PARAM_CHK((65535 < sport), RT_ERR_RG_INVALID_PARAM);

	if ((ret = reg_field_write(RTL9603D_CFG_L2TP_SPORTr, RTL9603D_CFG_L2TP_UDP_SPORTf, (uint32 *)&sport)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_l2UcAct_set(rtk_rg_l2UcActType_t actType)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	/* parameter check */
	RT_PARAM_CHK((FB_L2UCACT_END <= actType), RT_ERR_INPUT);
	
	if ((ret = reg_field_write(RTL9603D_LUT_CFGr, RTL9603D_LUT_L2UC_ACTf, (uint32 *)&actType)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_l2UcWanEn_set(uint32 portMask)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_port_idx_t port;
	uint32 tmpVal;
	
	HAL_SCAN_ALL_PORT(port)
	{
		if(portMask & (1<<port))
			tmpVal = 1;
		else
			tmpVal = 0;

		if ((ret = reg_array_field_write(RTL9603D_LUT_WAN_ENr, port, REG_ARRAY_INDEX_NONE, RTL9603D_ENf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

int32 rtk_rg_asic_reg_write(uint32 reg, uint32 value)
{
	uint32 writeVal = value;
	return reg_write(reg, &writeVal);
}

int32 rtk_rg_asic_reg_read(uint32 reg, uint32 *pValue)
{
	return reg_read(reg, pValue);
}


rtk_rg_err_code_t rtk_rg_asic_l2HsbaLatchMode_set(rtk_enable_t state)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 value  = 0;

	if(state == ENABLED)
		value = 0x0;
	else
		value = 0x1;

	/* function body */
	if ((ret = reg_field_write(RTL9603D_HSB_CTRLr, RTL9603D_LATCH_MODEf, &value)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	if ((ret = reg_field_write(RTL9603D_FBHSA_CTRLr, RTL9603D_LATCH_FB_MODEf, &value)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_hsbaMode_set(rtk_l34_hsba_mode_t hsbaMode)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "hsbaMode=%d",hsbaMode);

	/* parameter check */
	RT_PARAM_CHK((L34_HSBA_LOG <=hsbaMode), RT_ERR_INPUT);

	/* function body */
	if ((ret = reg_field_write(RTL9603D_HSBA_CTRLr, RTL9603D_TST_LOG_MDf, (uint32 *)&hsbaMode)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_hsbaMode_get(rtk_l34_hsba_mode_t *pHsbaMode)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmp_val;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pHsbaMode), RT_ERR_NULL_POINTER);

	/* function body */
	if ((ret = reg_field_read(RTL9603D_HSBA_CTRLr, RTL9603D_TST_LOG_MDf, &tmp_val)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pHsbaMode = tmp_val;

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_hsbDatav1_get(rtk_rg_asic_hsb_entry_t *pHsbData)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 index, tmp_val,*tmp_val_ptr;
	rtl9603d_asic_hsb_entry_t hsba_entry,tmp_hsba_entry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "");

	/* parameter check */
	RT_PARAM_CHK((NULL == pHsbData), RT_ERR_RG_NULL_POINTER);

	osal_memset(pHsbData, 0x0, sizeof(rtk_rg_asic_hsb_entry_t));

	/* function body */
	/*read data from register*/
	tmp_val_ptr = (uint32 *) &tmp_hsba_entry;
	for(index=0 ; index<(sizeof(rtl9603d_asic_hsb_entry_t)/4) ; index++)
	{
		if ((ret = reg_array_read(RTL9603D_HSB_DESCr, REG_ARRAY_INDEX_NONE, index, tmp_val_ptr)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_HWMISC|MOD_DAL), "");
			return ret;
		}
		tmp_val_ptr ++;
	}

	/*Prepare the data for reading*/
	for(index=0 ; index< (sizeof(rtl9603d_asic_hsb_entry_t)/4) ; index++)
	{
		hsba_entry.entry_data[sizeof(rtl9603d_asic_hsb_entry_t)/4 - 1 - index] = tmp_hsba_entry.entry_data[index];
	}

	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_DPORTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_DPORT = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_SPORTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_SPORT = tmp_val;

	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_L4_CSOKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_L4_CSOK = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_L4_PTCtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_L4_PTC = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_L4_TYPEtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_L4_TYPE = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_L3_CSOKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_L3_CSOK = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_IPMFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_IPMF = tmp_val;

	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_IP_OPTIONtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_IP_OPTION = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_TTL_STtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_TTL_ST = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_OUT_TOStf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->OUT_TOS = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L2_LEN_DIFFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2_LEN_DIFF = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_GRE_SEQtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->GRE_SEQ = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L2TP_IDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2TP_ID = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L2TP_SESSIONtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2TP_SESSION = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L2TPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2TP = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_GREtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->GRE = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_UDP_NOCStf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->UDP_NOCS = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_TCP_FLAGtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->TCP_FLAG = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DPORTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DPORT = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SPORTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SPORT = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L4_CSOKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L4_CSOK = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L4_PTCtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L4_PTC = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L4_TYPEtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L4_TYPE = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DIP_HSHtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DIP_HSH = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SIP_HSHtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SIP_HSH = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DIP_V4tf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DIP_V4 = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SIP_V4tf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SIP_V4 = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_IPV6_OUTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->IPV6_OUT = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L3_CSOKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L3_CSOK = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_IPMFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->IPMF = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_IP_OPTIONtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->IP_OPTION = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_TTL_STtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->TTL_ST = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_TOStf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->TOS = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_IPV4_6tf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->IPV4_6 = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_IPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->IP = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DUAL_FAILtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DUAL_FAIL = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DUAL_HDRtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DUAL_HDR = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_L2_LENtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2_LEN = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SA_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SA_IDX = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_DA_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->DA_IDX = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_PPPOE_IDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->PPPOE_SID = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SVLAN_IDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SVLAN_ID = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_CVLAN_IDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->CVLAN_ID= tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_PPPOE_IFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->PPPOE_IF = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_STAG_IFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->STAG_IF = tmp_val;

	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_CPRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->CPRI = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_CTAG_IFtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->CTAG_IF = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_ETH_TYPEtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->ETH_TYPE = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_STM_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->STM_IDX = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SPA_EXTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SPA_EXT = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_SPAtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->SPA = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_PADDINGtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->PADDING = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_BUF_FULLtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->L2_BUF_FULL = tmp_val;
	
	if ((ret = table_field_get(RTL9603D_L34_HSBt, RTL9603D_L34_HSB_RNG_NATHSB_GMAC_HITtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsbData->GMAC_CHK = tmp_val;
		
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_hsaDatav1_get(rtk_rg_asic_hsa_entry_t *pHsaData)
{
	int32 ret;
	int32 index;
	uint32 tmp_val,*tmp_val_ptr;
	rtl9603d_asic_hsa_entry_t hsba_entry,tmp_hsba_entry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "");

	/* parameter check */
	RT_PARAM_CHK((NULL == pHsaData), RT_ERR_NULL_POINTER);
	
	osal_memset(pHsaData, 0x0, sizeof(rtk_rg_asic_hsa_entry_t));

	/* function body */
	/*read data from register*/
	tmp_val_ptr = (uint32 *) &tmp_hsba_entry;
	for(index=0 ; index<(sizeof(rtl9603d_asic_hsa_entry_t)/4) ; index++)
	{
		if ((ret = reg_array_read(RTL9603D_HSA_DESCr, REG_ARRAY_INDEX_NONE, index, tmp_val_ptr)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_HWMISC|MOD_DAL), "");
			return ret;
		}
		tmp_val_ptr ++;
	}

	/* prepare data for reading */
	for(index=0 ; index< (sizeof(rtl9603d_asic_hsa_entry_t)/4) ; index++)
	{
		hsba_entry.entry_data[sizeof(rtl9603d_asic_hsa_entry_t)/4 - 1 - index] = tmp_hsba_entry.entry_data[index];
	}

	/*get field data from hsba buffer*/
	if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return RT_ERR_FAILED;
	}
	pHsaData->HSA_ACT = tmp_val ;

	if((pHsaData->HSA_ACT == FB_ACTION_TRAP2CPU) || (pHsaData->HSA_ACT == FB_ACTION_DROP)){
		/* Case 1. TRAP or DROP */
		if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_HIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_HID = tmp_val ;
		
		if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_HID_VLDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_HID_VLD = tmp_val ;
		
		if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_PRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_PRI = tmp_val ;
		
		if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_PRI_ENtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_PRI_EN = tmp_val ;
		
		if ((ret = table_field_get(RTL9603D_L34_HSA_TRAP_DROPt, RTL9603D_L34_HSA_TRAP_DROP_RNG_NATHSA_HSA_RSNtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_RSN = tmp_val ;
	}else if(pHsaData->HSA_ACT == FB_ACTION_FORWARD){
		/* Path 5 & Non-path5 shared data */
		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_P5tf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_P5 = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_SMAC_Ttf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_SMAC_T = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_EXTP_MASKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_EXTP_MASK = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_PMASKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_PMASK = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_EX_TAG_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_EX_TAG_IDX = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_PP_SIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_PP_SID = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_PP_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_PP_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_O_IF_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_O_IF_IDX = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_USER_PRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_USER_PRI = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_USER_PRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_USER_PRI_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_DSCPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_DSCP = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_DSCP_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_DSCP_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_SPRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_SPRI = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_SVIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_SVID = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_EGS_SVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_EGS_SVID_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_SPRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_SPRI_ACT= tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_SVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_SVID_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_STAG_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_STAG_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_VID2S_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_VID2S_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_CPRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_CPRI = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_CVIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_CVID = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_EGS_CVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_EGS_CVID_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_CPRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_CPRI_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_CVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_CVID_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_CTAG_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_CTAG_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_VID2C_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->S1_VID2C_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_MIB_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_FLOW_COUNTER_IDX = tmp_val ;
		
		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_MIB_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_FLOW_COUNTER_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_DMAC_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_DMAC_IDX = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_DMAC_Ttf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_DMAC_T = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_STREAM_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_STREAM_IDX = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_STREAM_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_STREAM_ACT = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_UC_LUT_LUPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_UC_LUT_LUP = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_I_IF_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_I_IF_IDX = tmp_val ;

		if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_HSA_HIT_DUALtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return RT_ERR_FAILED;
		}
		pHsaData->HSA_HIT_DUAL = tmp_val ;

		if(pHsaData->S1_P5){
			/* Case2. Path5: single action */
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_L4_CStf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_L4_CS = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_L3_CStf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_L3_CS = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_PORTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_PORT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_IPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_IP = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_L4_DIRtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_L4_DIR = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_PATH5_RNG_NATHSA_S1_L4_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S1_L4_ACT = tmp_val ;
		}else{
			/* Case3. Non Path5: may be multiple actions */
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_SMAC_Ttf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_SMAC_T = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_EXTP_MASKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_EXTP_MASK = tmp_val ;

			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_PMASKtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_PMASK = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_PP_SIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_PP_SID = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_PP_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_PP_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_O_IF_IDXtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_O_IF_IDX = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_USER_PRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_USER_PRI = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_USER_PRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_USER_PRI_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_DSCPtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_DSCP = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_DSCP_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_DSCP_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_SPRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_SPRI = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_SVIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_SVID = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_EGS_SVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_EGS_SVID_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_SPRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_SPRI_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_SVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_SVID_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_STAG_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_STAG_ACT= tmp_val ;

			if ((ret = table_field_get(RTL9603D_L34_HSA_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_VID2S_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_VID2S_ACT = tmp_val ;
	
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_CPRItf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_CPRI = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_CVIDtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_CVID = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_EGS_CVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_EGS_CVID_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_CPRI_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_CPRI_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_CVID_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_CVID_ACT= tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_CTAG_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_CTAG_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_VID2C_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_VID2C_ACT = tmp_val ;
			
			if ((ret = table_field_get(RTL9603D_L34_HSA_NON_PATH5t, RTL9603D_L34_HSA_NON_PATH5_RNG_NATHSA_S2_ACTtf, (uint32 *)&tmp_val, (uint32 *) &hsba_entry)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return RT_ERR_FAILED;
			}
			pHsaData->S2_ACT = tmp_val ;
		}
	}
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_hsbData_get(rtk_rg_asic_hsb_entry_t *pHsbData)
{
	return rtk_rg_asic_hsbDatav1_get(pHsbData);
}

rtk_rg_err_code_t rtk_rg_asic_hsaData_get(rtk_rg_asic_hsa_entry_t *pHsaData)
{
	return rtk_rg_asic_hsaDatav1_get(pHsaData);
}

rtk_rg_err_code_t rtk_rg_asic_sramFlowEntry_get(uint32 idx, void *pFlowData)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	u32 targetTable = 0;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);

	/*input error check*/
	RT_PARAM_CHK((pFlowData==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pFlowData, 0x0, sizeof(rtk_rg_asic_path1_entry_t));

	if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
		targetTable = RTL9603D_FLOW_TABLE_PATH1_2t;
	}
	else{
		targetTable = RTL9603D_TCAMt;
		idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
	}
	
	if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pFlowData)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_flowPath_del(uint32 idx)
{
	rtk_rg_asic_path1_entry_t P1P2Data;
	bzero(&P1P2Data, sizeof(rtk_rg_asic_path1_entry_t));

	/* Use path1 structure as empty buffer */
	ASSERT_EQ(rtk_rg_asic_flowPath1_set(idx, &P1P2Data), RT_ERR_RG_OK);

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath1_add(uint32 *idx, rtk_rg_asic_path1_entry_t *pP1Data, uint16 igrSVID, uint16 igrCVID)
{
	u32 entryIdx = 0, extraItem = 0, hashparam1 = 0, hashparam2 = 0;
	rtk_enable_t enabled = DISABLED;
	
	ASSERT_EQ(pP1Data->in_svlan_id, igrSVID);
	ASSERT_EQ(pP1Data->in_cvlan_id, igrCVID);

	extraItem = _rtk_rg_flowHashPath12ExtraItem_get(pP1Data);

	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH12_SKIP_SVID, &enabled), RT_ERR_RG_OK);
	if(!enabled) hashparam1 = igrSVID; 
	else hashparam1 = 0;
	
	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH12_SKIP_CVID, &enabled), RT_ERR_RG_OK);
	if(!enabled) hashparam2 = igrCVID;
	else hashparam2 = 0;
	/* Extraitem: Consider L4 protocol only for path 3/4/5 */
	// NA
	
	entryIdx = _rtk_rg_flowHashIndexStep1_get(hashparam1, hashparam2, pP1Data->in_smac_lut_idx, pP1Data->in_dmac_lut_idx, extraItem);

	// Default setting assignment
	pP1Data->valid = TRUE;
	pP1Data->in_path = 0;
	pP1Data->in_multiple_act = FALSE;
	if(pP1Data->in_out_stream_idx_check_act==0) pP1Data->in_out_stream_idx = 0;
	if(pP1Data->in_ctagif==0) pP1Data->in_cvlan_id = 0;
	if(pP1Data->in_stagif==0) pP1Data->in_svlan_id = 0;
	if(pP1Data->in_pppoeif==0) {pP1Data->in_pppoe_sid= 0; pP1Data->in_pppoe_sid_check=0;}
	
	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;
	
	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath1_set(entryIdx, pP1Data), RT_ERR_RG_OK);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath1_set(uint32 idx, rtk_rg_asic_path1_entry_t *pP1Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();
		
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);

	/*input error check*/
	RT_PARAM_CHK((pP1Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path1_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH1_2t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		}
		
		if ((ret = rg_asic_table_write(targetTable, idx, (uint32 *)pP1Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		if(targetTable == RTL9603D_TCAMt){
			/* For sync data from TCAM SRAM to TCAM Compare, first time will write care array, second time will write data array */
			/* To write data array, use index bit 6 to indicate different array.  0: care array; 1: data array.*/
			if ((ret = rg_asic_table_write(targetTable, (idx | RTL9603D_FLOW_TCAM_DATABIT), (uint32 *)pP1Data)) != RT_ERR_RG_OK)
			{
			    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			    	return RT_ERR_FAILED;
			}
		}
		_rtk_rg_flowEntryValidBit_set(pP1Data->valid, (targetTable == RTL9603D_TCAMt)?idx+RTL9603D_TABLESIZE_FLOWSRAM:idx);
	}else{ 
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP1Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath1_get(uint32 idx, rtk_rg_asic_path1_entry_t *pP1Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);

	/*input error check*/
	RT_PARAM_CHK((pP1Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path1_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP1Data, 0x0, sizeof(rtk_rg_asic_path1_entry_t));

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH1_2t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
			idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
		}
		
		if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pP1Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP1Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath2_add(uint32 *idx, rtk_rg_asic_path2_entry_t *pP2Data, uint16 igrSVID, uint16 igrCVID)
{
	u32 entryIdx = 0, extraItem = 0, hashparam1 = 0, hashparam2 = 0;
	rtk_enable_t enabled = DISABLED;
	
	ASSERT_EQ(pP2Data->in_svlan_id, igrSVID);
	ASSERT_EQ(pP2Data->in_cvlan_id, igrCVID);
	
	extraItem = _rtk_rg_flowHashPath12ExtraItem_get(pP2Data);

	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH12_SKIP_SVID, &enabled), RT_ERR_RG_OK);
	if(!enabled) hashparam1 = igrSVID; 
	else hashparam1 = 0;
	
	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH12_SKIP_CVID, &enabled), RT_ERR_RG_OK);
	if(!enabled) hashparam2 = igrCVID;
	else hashparam2 = 0;
	/* Extraitem: Consider L4 protocol only for path 3/4/5 */
	// NA
	
	entryIdx = _rtk_rg_flowHashIndexStep1_get(hashparam1, hashparam2, pP2Data->in_smac_lut_idx, pP2Data->in_dmac_lut_idx, extraItem);

	// Default setting assignment
	pP2Data->valid = TRUE;
	pP2Data->in_path = 0;
	pP2Data->in_multiple_act = TRUE;
	if(pP2Data->in_ctagif==0) pP2Data->in_cvlan_id = 0;
	if(pP2Data->in_stagif==0) pP2Data->in_svlan_id = 0;
	if(pP2Data->in_pppoeif==0) {pP2Data->in_pppoe_sid= 0; pP2Data->in_pppoe_sid_check=0;}
	
	/* Search if there is conflict egress port in corresponding path1 entry */
	if((_rtk_rg_flowEntryConflictEgrCPUPortChecking(pP2Data, entryIdx)) == FAILED)
		return RT_ERR_RG_FAILED;
	
	/* Path2: step1 index should translate to step2 index */
	entryIdx = _rtk_rg_flowHashIndexStep2_get(entryIdx);
	
	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;
	
	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath2_set(entryIdx, pP2Data), RT_ERR_RG_OK);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath2_set(uint32 idx, rtk_rg_asic_path2_entry_t *pP2Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP2Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path2_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH1_2t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		}
			
		if ((ret = rg_asic_table_write(targetTable, idx, (uint32 *)pP2Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		if(targetTable == RTL9603D_TCAMt){
			/* For sync data from TCAM SRAM to TCAM Compare, first time will write care array, second time will write data array */
			/* To write data array, use index bit 6 to indicate different array.  0: care array; 1: data array.*/
			if ((ret = rg_asic_table_write(targetTable, (idx | RTL9603D_FLOW_TCAM_DATABIT), (uint32 *)pP2Data)) != RT_ERR_RG_OK)
			{
			    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			    	return RT_ERR_FAILED;
			}
		}
		_rtk_rg_flowEntryValidBit_set(pP2Data->valid, (targetTable == RTL9603D_TCAMt)?idx+RTL9603D_TABLESIZE_FLOWSRAM:idx);
	}else{
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP2Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath2_get(uint32 idx, rtk_rg_asic_path2_entry_t *pP2Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP2Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path2_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP2Data, 0x0, sizeof(rtk_rg_asic_path2_entry_t));

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH1_2t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
			idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
		}
		
		if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pP2Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP2Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}
	
rtk_rg_err_code_t rtk_rg_asic_flowPath3_add(uint32 *idx, rtk_rg_asic_path3_entry_t *pP3Data, uint16 igrSVID, uint16 igrCVID)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	if ((ret = rtk_rg_asic_flowPath3DAHash_add(idx, pP3Data, igrSVID, igrCVID, 0)) != RT_ERR_RG_OK)
	{
	    	WARNING("Add flow path3 entry fail, ret = %d", ret);
	    	return ret;
	}

	return RT_ERR_RG_OK;
}

/*
 * Input:
 *      lutDaIdx  - for v1: lutDaIdx[11:0]. for v2: gmacChk[0:0]
*/
rtk_rg_err_code_t rtk_rg_asic_flowPath3DAHash_add(uint32 * idx, rtk_rg_asic_path3_entry_t * pP3Data, uint16 igrSVID, uint16 igrCVID, uint16 lutDaIdx)
{
	u32 entryIdx = 0, extraItem = 0;

	extraItem = _rtk_rg_flowHashPath34ExtraItem_get(pP3Data, igrSVID, igrCVID, lutDaIdx);
	
	entryIdx = _rtk_rg_flowHashIndexStep1_get(pP3Data->in_l4_src_port, pP3Data->in_l4_dst_port, pP3Data->in_src_ipv4_addr, pP3Data->in_dst_ipv4_addr, extraItem);

	// Default setting assignment
	pP3Data->valid = TRUE;
	pP3Data->in_path = 1;
	pP3Data->in_multiple_act = FALSE;
	
	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;

	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath3_set(entryIdx, pP3Data), RT_ERR_RG_OK);

	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_flowPath3_set(uint32 idx, rtk_rg_asic_path3_entry_t *pP3Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP3Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path3_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH3_4t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		}
			
		if ((ret = rg_asic_table_write(targetTable, idx, (uint32 *)pP3Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		if(targetTable == RTL9603D_TCAMt){
			/* For sync data from TCAM SRAM to TCAM Compare, first time will write care array, second time will write data array */
			/* To write data array, use index bit 6 to indicate different array.  0: care array; 1: data array.*/
			if ((ret = rg_asic_table_write(targetTable, (idx | RTL9603D_FLOW_TCAM_DATABIT), (uint32 *)pP3Data)) != RT_ERR_RG_OK)
			{
			    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			    	return RT_ERR_FAILED;
			}
		}
		_rtk_rg_flowEntryValidBit_set(pP3Data->valid, (targetTable == RTL9603D_TCAMt)?idx+RTL9603D_TABLESIZE_FLOWSRAM:idx);
	}else{
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP3Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath3_get(uint32 idx, rtk_rg_asic_path3_entry_t *pP3Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP3Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path3_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP3Data, 0x0, sizeof(rtk_rg_asic_path3_entry_t));

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH3_4t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
			idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
		}
		
		if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pP3Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP3Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath4_add(uint32 *idx, rtk_rg_asic_path4_entry_t *pP4Data, uint16 igrSVID, uint16 igrCVID)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	if ((ret = rtk_rg_asic_flowPath4DAHash_add(idx, pP4Data, igrSVID, igrCVID, 0)) != RT_ERR_RG_OK)
	{
	    	WARNING("Add flow path4 entry fail, ret = %d", ret);
	    	return ret;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath4DAHash_add(uint32 * idx, rtk_rg_asic_path4_entry_t * pP4Data, uint16 igrSVID, uint16 igrCVID, uint16 lutDaIdx)
{
	u32 entryIdx = 0, extraItem = 0;
	
	extraItem = _rtk_rg_flowHashPath34ExtraItem_get(pP4Data, igrSVID, igrCVID, lutDaIdx);
	
	entryIdx = _rtk_rg_flowHashIndexStep1_get(pP4Data->in_l4_src_port, pP4Data->in_l4_dst_port, pP4Data->in_src_ipv4_addr, pP4Data->in_dst_ipv4_addr, extraItem);

	// Default setting assignment
	pP4Data->valid = TRUE;
	pP4Data->in_path = 1;
	pP4Data->in_multiple_act = TRUE;
	
	/* Search if there is conflict egress port in corresponding path3 entry */
	if((_rtk_rg_flowEntryConflictEgrCPUPortChecking(pP4Data, entryIdx)) == FAILED)
		return RT_ERR_RG_FAILED;
	
	/* Path4: step1 index should translate to step2 index */
	entryIdx = _rtk_rg_flowHashIndexStep2_get(entryIdx);

	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;

	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath4_set(entryIdx, pP4Data), RT_ERR_RG_OK);

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath4_set(uint32 idx, rtk_rg_asic_path4_entry_t *pP4Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP4Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path4_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH3_4t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		}
			
		if ((ret = rg_asic_table_write(targetTable, idx, (uint32 *)pP4Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		if(targetTable == RTL9603D_TCAMt){
			/* For sync data from TCAM SRAM to TCAM Compare, first time will write care array, second time will write data array */
			/* To write data array, use index bit 6 to indicate different array.  0: care array; 1: data array.*/
			if ((ret = rg_asic_table_write(targetTable, (idx | RTL9603D_FLOW_TCAM_DATABIT), (uint32 *)pP4Data)) != RT_ERR_RG_OK)
			{
			    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			    	return RT_ERR_FAILED;
			}
		}
		_rtk_rg_flowEntryValidBit_set(pP4Data->valid, (targetTable == RTL9603D_TCAMt)?idx+RTL9603D_TABLESIZE_FLOWSRAM:idx);
	}else{
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP4Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath4_get(uint32 idx, rtk_rg_asic_path4_entry_t *pP4Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP4Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path4_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP4Data, 0x0, sizeof(rtk_rg_asic_path4_entry_t));

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH3_4t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
			idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
		}
		
		if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pP4Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP4Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath5_add(uint32 *idx, rtk_rg_asic_path5_entry_t *pP5Data, uint16 igrSVID, uint16 igrCVID)
{
	u32 entryIdx = 0, extraItem = 0;	
	
	extraItem = _rtk_rg_flowHashPath5ExtraItem_get(pP5Data, igrSVID, igrCVID);
	
	if(pP5Data->in_ipv4_or_ipv6==0 && pP5Data->out_l4_act==1 && pP5Data->out_l4_direction==0){	
		/* NAPTR: get dest ip address from interface gateway ip */
		rtk_rg_asic_netif_entry_t netIf;
		bzero(&netIf, sizeof(rtk_rg_asic_netif_entry_t));
		rtk_rg_asic_netifTable_get(pP5Data->in_intf_idx, &netIf);
		entryIdx = _rtk_rg_flowHashIndexStep1_get(pP5Data->in_l4_src_port, pP5Data->in_l4_dst_port, pP5Data->in_src_ipv4_addr, netIf.gateway_ipv4_addr, extraItem);
	}else
		entryIdx = _rtk_rg_flowHashIndexStep1_get(pP5Data->in_l4_src_port, pP5Data->in_l4_dst_port, pP5Data->in_src_ipv4_addr, pP5Data->in_dst_ipv4_addr, extraItem);

	// Default setting assignment
	pP5Data->valid = TRUE;
	pP5Data->in_path = 2;
	
	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;
	
	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath5_set(entryIdx, pP5Data), RT_ERR_RG_OK);	
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath5_set(uint32 idx, rtk_rg_asic_path5_entry_t *pP5Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP5Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path5_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH5t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
		}
			
		if ((ret = rg_asic_table_write(targetTable, idx, (uint32 *)pP5Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		if(targetTable == RTL9603D_TCAMt){
			/* For sync data from TCAM SRAM to TCAM Compare, first time will write care array, second time will write data array */
			/* To write data array, use index bit 6 to indicate different array.  0: care array; 1: data array.*/
			if ((ret = rg_asic_table_write(targetTable, (idx | RTL9603D_FLOW_TCAM_DATABIT), (uint32 *)pP5Data)) != RT_ERR_RG_OK)
			{
			    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			    	return RT_ERR_FAILED;
			}
		}
		_rtk_rg_flowEntryValidBit_set(pP5Data->valid, (targetTable == RTL9603D_TCAMt)?idx+RTL9603D_TABLESIZE_FLOWSRAM:idx);
	}else{
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP5Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath5_get(uint32 idx, rtk_rg_asic_path5_entry_t *pP5Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP5Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path5_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP5Data, 0x0, sizeof(rtk_rg_asic_path5_entry_t));

	if(fbMode == FB_MODE_4K){
		u32 targetTable = 0;
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM + RTL9603D_TABLESIZE_FLOWTCAM <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
		if(idx < RTL9603D_TABLESIZE_FLOWSRAM){
			targetTable = RTL9603D_FLOW_TABLE_PATH5t;
		}
		else{
			targetTable = RTL9603D_TCAMt;
			idx -= RTL9603D_TABLESIZE_FLOWSRAM;
			idx |= RTL9603D_FLOW_TCAM_DATABIT;	// set as data array to get tcam sram data
		}
		
		if ((ret = rg_asic_table_read(targetTable, idx, (uint32 *)pP5Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP5Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath6_add(uint32 *idx, rtk_rg_asic_path6_entry_t *pP6Data, uint16 igrSVID, uint16 igrCVID)
{
	u32 entryIdx = 0, extraItem = 0;
	rtk_enable_t enabled = DISABLED;
	u16 saIdx = 0, daIdx = 0;
	
	extraItem = _rtk_rg_flowHashPath6ExtraItem_get(pP6Data, igrSVID, igrCVID);
	
	/* Consider L4 protocol only for path 3/4/5 */
	// NA

	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH6_SKIP_SA, &enabled), RT_ERR_RG_OK);
	if(!enabled )	saIdx = pP6Data->in_smac_lut_idx;
	
	ASSERT_EQ(rtk_rg_asic_globalState_get(FB_GLOBAL_PATH6_SKIP_DA, &enabled), RT_ERR_RG_OK);
	if(!enabled )	daIdx = pP6Data->in_dmac_lut_idx;
	
	
	entryIdx = _rtk_rg_flowHashIndexStep1_get(saIdx, daIdx, pP6Data->in_src_ipv4_addr, pP6Data->in_dst_ipv4_addr, extraItem);

	// Default setting assignment
	pP6Data->valid = TRUE;
	pP6Data->in_path = 3;
	
	/* search 4-way or 1-way hash entries and get one invalid(free) entry */
	if((entryIdx = _rtk_rg_flowEntryAvailableIdx_get(entryIdx)) == FAILED)
		return RT_ERR_RG_ENTRY_FULL;
	
    	/* Path6 didn't support TCAM access, check index range in set function later */
	
	*idx = entryIdx;
	
	ASSERT_EQ(rtk_rg_asic_flowPath6_set(entryIdx, pP6Data), RT_ERR_RG_OK);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath6_set(uint32 idx, rtk_rg_asic_path6_entry_t *pP6Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP6Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path6_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if(fbMode == FB_MODE_4K){
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM<=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
			
		if ((ret = rg_asic_table_write(RTL9603D_FLOW_TABLE_PATH6t, idx, (uint32 *)pP6Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		_rtk_rg_flowEntryValidBit_set(pP6Data->valid, idx);
	}else{
		/* DDR mode - write entry to specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryWriteToDDR(idx, pP6Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowMib_get(uint32 idx, rtk_rg_asic_flowMib_entry_t *pFlowMib)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pFlowMib==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWMIB<=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/*Ingress flow counter*/
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34FLOWOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->in_byte_cnt = tmpVal;
	pFlowMib->in_byte_cnt = (pFlowMib->in_byte_cnt<<32);
	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34FLOWOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->in_byte_cnt += tmpVal;

	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34FLOWPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->in_packet_cnt = tmpVal;

	/*Egress flow counter*/
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34FLOWOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->out_byte_cnt = tmpVal;
	pFlowMib->out_byte_cnt = (pFlowMib->out_byte_cnt<<32);
	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34FLOWOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->out_byte_cnt += tmpVal;

	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_FLOW_MIBr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34FLOWPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	pFlowMib->out_packet_cnt = tmpVal;	
	
	return ret;
}

rtk_rg_err_code_t rtk_rg_asic_flowMib_reset(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, retry;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWMIB<=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/*Check if ASIC is still reseting MIB or not*/
	if ((ret = reg_field_read(RTL9603D_STAT_RST_CFGr, RTL9603D_BUSY_STATf, &tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return ret;
	}

	if (tmpVal != 0)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_BUSYWAIT_TIMEOUT;
	}

	tmpVal = 1;
	if ((ret = reg_array_field_write(RTL9603D_STAT_L34_FLOW_RSTr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}	
	
	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_STAT_RST_CFGr, RTL9603D_RST_CMDf, &tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}

	/* Wait operation completed */
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_STAT_RST_CFGr, RTL9603D_BUSY_STATf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;  

	// clear reset request by manual
	tmpVal = 0;
	if ((ret = reg_array_field_write(RTL9603D_STAT_L34_FLOW_RSTr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}	
	
	
	return ret;
}

rtk_rg_err_code_t rtk_rg_asic_flowPath6_get(uint32 idx, rtk_rg_asic_path6_entry_t *pP6Data)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "index %d", idx);
	
	/*input error check*/
	RT_PARAM_CHK((pP6Data==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_flow_entry_t)!=sizeof(rtk_rg_asic_path6_entry_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(pP6Data, 0x0, sizeof(rtk_rg_asic_path6_entry_t));

	if(fbMode == FB_MODE_4K){
		RT_PARAM_CHK((RTL9603D_TABLESIZE_FLOWSRAM<=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = rg_asic_table_read(RTL9603D_FLOW_TABLE_PATH6t, idx, (uint32 *)pP6Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}else{
		/* DDR mode - read entry from specific memory address */
		uint32 entryNum = _rtk_rg_flowEntryNum_get();
		RT_PARAM_CHK((entryNum <=idx), RT_ERR_RG_INDEX_OUT_OF_RANGE);

		if ((ret = _rtk_rg_flowEntryReadFromDDR(idx, pP6Data)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifTable_add(uint32 idx, rtk_rg_asic_netif_entry_t *pNetifEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	rtl9603d_asic_netif_entry_t netif_entry;

	/*input error check*/
	RT_PARAM_CHK((pNetifEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);
	RT_PARAM_CHK((pNetifEntry->valid != 0 && pNetifEntry->valid != 1), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->intf_mtu >= 16384), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->intf_mtu_check != 0 && pNetifEntry->intf_mtu_check != 1), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->out_pppoe_act >= FB_NETIFPPPOE_ACT_END), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->deny_ipv4 != 0 && pNetifEntry->deny_ipv4 != 1), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->deny_ipv6 != 0 && pNetifEntry->deny_ipv6 != 1), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->ingress_action >= FB_ACTION_END), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->egress_action >= FB_ACTION_END), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->allow_ingress_portmask.bits[0] >= (1 << HAL_GET_PORTNUM())), RT_ERR_INPUT);
	RT_PARAM_CHK((pNetifEntry->allow_ingress_ext_portmask.bits[0] >= (1 << (HAL_GET_MAX_EXT_PORT()+1))), RT_ERR_INPUT);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&netif_entry, 0x0, sizeof(netif_entry));

	tmpVal = pNetifEntry->deny_ipv4;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_DENY_IPV4tf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}

	tmpVal =pNetifEntry->deny_ipv6;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_DENY_IPV6tf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->egress_action;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_EGS_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	
	if ((ret = rg_table_field_mac_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_GMACtf, (uint8 *)&(pNetifEntry->gateway_mac_addr), (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->gateway_ipv4_addr;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_GIPtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	
	tmpVal = pNetifEntry->ingress_action;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->allow_ingress_ext_portmask.bits[0];
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_EXT_PMSKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->allow_ingress_portmask.bits[0];
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_PMSKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->intf_mtu;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_MTUtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->intf_mtu_check;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_MTU_CHKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->out_pppoe_act;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_PPPOE_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->out_pppoe_sid;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_PPPOE_IDtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	tmpVal = pNetifEntry->valid;
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_VALIDtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	if ((ret = rg_asic_table_write(RTL9603D_INTERFACEt, idx, (uint32 *)&netif_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifTable_del(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtl9603d_asic_netif_entry_t netif_entry;
	uint32 is_valid=0;
	//printk("apolloPro_asicDriver.c %s\n", __FUNCTION__);
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&netif_entry, 0x0, sizeof(netif_entry));
	
	if ((ret = rg_table_field_set(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_VALIDtf, (uint32 *)&is_valid, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	if ((ret = rg_asic_table_write(RTL9603D_INTERFACEt, idx, (uint32 *)&netif_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifTable_get(uint32 idx, rtk_rg_asic_netif_entry_t *pNetifEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtl9603d_asic_netif_entry_t netif_entry;
	uint32 tmpVal;
	//printk("apolloPro_asicDriver.c %s line:%d\n", __FUNCTION__, __LINE__);

	RT_PARAM_CHK((pNetifEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&netif_entry, 0x0, sizeof(netif_entry));

	if ((ret = rg_asic_table_read(RTL9603D_INTERFACEt, idx, (uint32 *)&netif_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_DENY_IPV4tf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	pNetifEntry->deny_ipv4 = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_DENY_IPV6tf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->deny_ipv6 = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_EGS_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->egress_action = tmpVal;

	if ((ret = rg_table_field_mac_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_GMACtf, (uint8 *)&(pNetifEntry->gateway_mac_addr), (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_GIPtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->gateway_ipv4_addr = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->ingress_action = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_EXT_PMSKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->allow_ingress_ext_portmask.bits[0] = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_IGS_PMSKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->allow_ingress_portmask.bits[0] = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_MTUtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->intf_mtu = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_MTU_CHKtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->intf_mtu_check = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_PPPOE_ACTtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->out_pppoe_act = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_PPPOE_IDtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->out_pppoe_sid = tmpVal;

	if ((ret = rg_table_field_get(RTL9603D_INTERFACEt, RTL9603D_INTERFACE_VALIDtf, (uint32 *)&tmpVal, (uint32 *) &netif_entry)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return RT_ERR_FAILED;
	}
	pNetifEntry->valid = tmpVal;
	
	//printk("apolloPro_asicDriver.c %s line:%d\n", __FUNCTION__, __LINE__);
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifMib_get(uint32 idx, rtk_rg_asic_netifMib_entry_t *pNetifMibEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/*input error check*/
	RT_PARAM_CHK((pNetifMibEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* IN BC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFBCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_bc_byte_cnt = tmpVal;
	pNetifMibEntry->in_intf_bc_byte_cnt = (pNetifMibEntry->in_intf_bc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFBCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_bc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFBCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_bc_packet_cnt = tmpVal;
	
	/* IN UC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFUCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_uc_byte_cnt = tmpVal;
	pNetifMibEntry->in_intf_uc_byte_cnt = (pNetifMibEntry->in_intf_uc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFUCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_uc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFUCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_uc_packet_cnt = tmpVal;

	/* IN MC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFMCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_mc_byte_cnt = tmpVal;
	pNetifMibEntry->in_intf_mc_byte_cnt = (pNetifMibEntry->in_intf_mc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFMCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_mc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_INL34IFMCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->in_intf_mc_packet_cnt = tmpVal;

	/* OUT BC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFBCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_bc_byte_cnt = tmpVal;
	pNetifMibEntry->out_intf_bc_byte_cnt = (pNetifMibEntry->out_intf_bc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFBCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_bc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_BCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFBCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_bc_packet_cnt = tmpVal;
	
	/* OUT UC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFUCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_uc_byte_cnt = tmpVal;
	pNetifMibEntry->out_intf_uc_byte_cnt = (pNetifMibEntry->out_intf_uc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFUCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_uc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_UCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFUCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_uc_packet_cnt = tmpVal;

	/* OUT MC */
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFMCOCTETS_Hf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_mc_byte_cnt = tmpVal;
	pNetifMibEntry->out_intf_mc_byte_cnt = (pNetifMibEntry->out_intf_mc_byte_cnt<<32);
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFMCOCTETS_Lf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_mc_byte_cnt += tmpVal;	
	if ((ret = reg_array_field_read(RTL9603D_STAT_L34_MIB_MCr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_OUTL34IFMCPKTSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	pNetifMibEntry->out_intf_mc_packet_cnt = tmpVal;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifMib_reset(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "netifMib reset %d", idx);

	/*input error check*/
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);


	/*Check if ASIC is still reseting MIB or not*/
	if ((ret = reg_field_read(RTL9603D_STAT_RST_CFGr, RTL9603D_BUSY_STATf, &tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return ret;
	}

	if (tmpVal != 0)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_BUSYWAIT_TIMEOUT;
	}

	tmpVal = 1;
	if ((ret = reg_array_field_write(RTL9603D_STAT_L34_RSTr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}	
	tmpVal = 1;
	if ((ret = reg_array_field_write(RTL9603D_STAT_L34_MC_RSTr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}
	tmpVal = 1;
	if ((ret = reg_array_field_write(RTL9603D_STAT_L34_BC_RSTr, REG_ARRAY_INDEX_NONE, idx, RTL9603D_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}


	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_STAT_RST_CFGr, RTL9603D_RST_CMDf, &tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		return ret;
	}

	/* Wait operation completed */
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_STAT_RST_CFGr, RTL9603D_BUSY_STATf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;  

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_indirectMacTable_add(uint32 idx, rtk_rg_asic_indirectMac_entry_t *pIndirectMacEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "IndirectMac %d = 0x%x", idx, pIndirectMacEntry->l2_idx);

	/*input error check*/
	RT_PARAM_CHK((pIndirectMacEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_MACIND<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_write(RTL9603D_MAC_IDXt, idx, (uint32 *)pIndirectMacEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_indirectMacTable_del(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtl9603d_asic_indMac_entry_t indMac_entry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "IndirectMac %d = 0x%x", idx);

	/*input error check*/
	RT_PARAM_CHK((RTL9603D_TABLESIZE_MACIND<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&indMac_entry, 0x0, sizeof(indMac_entry));

	if ((ret = rg_asic_table_write(RTL9603D_MAC_IDXt, idx, (uint32 *)&indMac_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_indirectMacTable_get(uint32 idx, rtk_rg_asic_indirectMac_entry_t *pIndirectMacEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	/*input error check*/
	RT_PARAM_CHK((pIndirectMacEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_MACIND<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_read(RTL9603D_MAC_IDXt, idx, (uint32 *)pIndirectMacEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "IndirectMac %d = 0x%x", idx, pIndirectMacEntry->l2_idx);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extPortMaskTable_add(uint32 idx, rtk_rg_asic_extPortMask_entry_t *pExtPMaskEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
#if defined(CONFIG_APOLLOPRO_FPGA)	
	rtk_portmask_t portMask;
#endif
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ExtPortMask %d = 0x%x", idx, pExtPMaskEntry->extpmask);

	/*input error check*/
	RT_PARAM_CHK((pExtPMaskEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_EXTPORT<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_write(RTL9603D_FB_EXT_PORTt, idx, (uint32 *)pExtPMaskEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
#if defined(CONFIG_APOLLOPRO_FPGA)
	/* Synchronize extension portmask table to LUT_EXT_MBR and VLAN_EXT_MBR */
	portMask.bits[0] = pExtPMaskEntry->extpmask;
	if ((ret = rtk_l2_extMemberConfig_set(idx, portMask)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	if ((ret = rtk_vlan_extPortmaskCfg_set(idx, &portMask)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
#endif
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extPortMaskTable_del(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtl9603d_asic_extPortMask_entry_t extPMask_entry;
#if defined(CONFIG_APOLLOPRO_FPGA)
	rtk_portmask_t portMask;
#endif	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ExtPortMask %d", idx);

	/*input error check*/
	RT_PARAM_CHK((RTL9603D_TABLESIZE_EXTPORT<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&extPMask_entry, 0x0, sizeof(extPMask_entry));

	if ((ret = rg_asic_table_write(RTL9603D_FB_EXT_PORTt, idx, (uint32 *)&extPMask_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
#if defined(CONFIG_APOLLOPRO_FPGA)
	/* Synchronize extension portmask table to LUT_EXT_MBR and VLAN_EXT_MBR */
	portMask.bits[0] = 0;
	if ((ret = rtk_l2_extMemberConfig_set(idx, portMask)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	if ((ret = rtk_vlan_extPortmaskCfg_set(idx, &portMask)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
#endif	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extPortMaskTable_get(uint32 idx, rtk_rg_asic_extPortMask_entry_t *pExtPMaskEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	/*input error check*/
	RT_PARAM_CHK((pExtPMaskEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_EXTPORT<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_read(RTL9603D_FB_EXT_PORTt, idx, (uint32 *)pExtPMaskEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ExtPortMask %d = 0x%x", idx, pExtPMaskEntry->extpmask);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_etherTypeTable_add(uint32 idx, rtk_rg_asic_etherType_entry_t *pEtherTypeEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "EtherType %d = 0x%x", idx, pEtherTypeEntry->ethertype);

	/*input error check*/
	RT_PARAM_CHK((pEtherTypeEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_ETHERTYPE<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_write(RTL9603D_ETHER_TYPEt, idx, (uint32 *)pEtherTypeEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_etherTypeTable_del(uint32 idx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtl9603d_asic_etherType_entry_t ethertype_entry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "EtherType del %d", idx);

	/*input error check*/
	RT_PARAM_CHK((RTL9603D_TABLESIZE_ETHERTYPE<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	osal_memset(&ethertype_entry, 0x0, sizeof(ethertype_entry));

	if ((ret = rg_asic_table_write(RTL9603D_ETHER_TYPEt, idx, (uint32 *)&ethertype_entry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_etherTypeTable_get(uint32 idx, rtk_rg_asic_etherType_entry_t *pEtherTypeEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	/*input error check*/
	RT_PARAM_CHK((pEtherTypeEntry==NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((RTL9603D_TABLESIZE_ETHERTYPE<=idx), RT_ERR_ENTRY_INDEX);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	if ((ret = rg_asic_table_read(RTL9603D_ETHER_TYPEt, idx, (uint32 *)pEtherTypeEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "EtherType %d = 0x%x", idx, pEtherTypeEntry->ethertype);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagAction_add(uint32 actionList, uint32 actionIdx, rtk_rg_asic_extraTagAction_t *pExtraTagAction)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 actionReg = RTL9603D_EXTG_ACTYPE0r;

	RT_PARAM_CHK((pExtraTagAction == NULL), RT_ERR_RG_NULL_POINTER);
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Extra tag list %d, act index %d, act bit = %d", actionList, actionIdx, pExtraTagAction->type1.act_bit);

	/*input error check*/
	RT_PARAM_CHK((actionList<RTL9603D_EXTRATAG_LISTMIN) || (actionList>RTL9603D_EXTRATAG_LISTMAX), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((actionIdx>=RTL9603D_EXTRATAG_ACTIONS), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_extraTag_entry_t)!=sizeof(rtk_rg_asic_extraTagAction_t)), RT_ERR_RG_BUF_OVERFLOW);

	if(pExtraTagAction->type1.act_bit == FB_EXTG_ACTBIT_1)
	{
		RT_PARAM_CHK(((pExtraTagAction->type1.src_addr_offset&0x3) != 0), RT_ERR_RG_INVALID_PARAM);
		RT_PARAM_CHK(((pExtraTagAction->type1.src_addr_offset+pExtraTagAction->type1.length) > 512), RT_ERR_RG_BUF_OVERFLOW);
	}

	/* check Init status */
	RT_INIT_CHK(fb_init);

	switch(actionIdx)
	{
	case 0:
		actionReg = RTL9603D_EXTG_ACTYPE0r;
		break;
	case 1:
		actionReg = RTL9603D_EXTG_ACTYPE1r;
		break;
	case 2:
		actionReg = RTL9603D_EXTG_ACTYPE2r;
		break;
	case 3:
		actionReg = RTL9603D_EXTG_ACTYPE3r;
		break;
	case 4:
		actionReg = RTL9603D_EXTG_ACTYPE4r;
		break;
	case 5:
		actionReg = RTL9603D_EXTG_ACTYPE5r;
		break;
	case 6:
		actionReg = RTL9603D_EXTG_ACTYPE6r;
		break;
	case 7:
		actionReg = RTL9603D_EXTG_ACTYPE7r;
		break;
	default:
		return RT_ERR_RG_FAILED;
		break;
		
	}

	if(pExtraTagAction->type4.act_bit == FB_EXTG_ACTBIT_4)
	{
		// prevent NOP action seting because HW couldn't guarantee the behavior.
		if((pExtraTagAction->type4.data_src_type==0) && (pExtraTagAction->type4.reduce_seq==1) && (pExtraTagAction->type4.reduce_ack==1))
		{
			WARNING("This extratag action is meanless(NOP), you could just skip this act_bit setting");
			return RT_ERR_RG_INVALID_PARAM;
		}
	}

	/* actionList 1~7 mapping to port(array) 0~6 */
	if ((ret = reg_array_write(actionReg, actionList-1, REG_ARRAY_INDEX_NONE, (uint32 *)pExtraTagAction)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}

	if(pExtraTagAction->type1.act_bit == FB_EXTG_ACTBIT_1){
		/* Assumption: one action list  could has one action bit 1 only!! */
		ASSERT_EQ(rtk_rg_asic_extraTagInsertHdrLen_set(actionList, pExtraTagAction->type1.length), SUCCESS);
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagAction_del(uint32 actionList, uint32 actionIdx)
{
	rtk_rg_asic_extraTagAction_t emptyExtraTag;
	bzero(&emptyExtraTag, sizeof(rtk_rg_asic_extraTagAction_t));

	ASSERT_EQ(rtk_rg_asic_extraTagAction_add(actionList, actionIdx, &emptyExtraTag), RT_ERR_RG_OK);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagAction_get(uint32 actionList, uint32 actionIdx, rtk_rg_asic_extraTagAction_t *pExtraTagAction)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 actionReg = RTL9603D_EXTG_ACTYPE0r;

	/*input error check*/
	RT_PARAM_CHK((pExtraTagAction == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((actionList<RTL9603D_EXTRATAG_LISTMIN) || (actionList>RTL9603D_EXTRATAG_LISTMAX), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((actionIdx>=RTL9603D_EXTRATAG_ACTIONS), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((sizeof(rtl9603d_asic_extraTag_entry_t)!=sizeof(rtk_rg_asic_extraTagAction_t)), RT_ERR_RG_BUF_OVERFLOW);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	switch(actionIdx)
	{
	case 0:
		actionReg = RTL9603D_EXTG_ACTYPE0r;
		break;
	case 1:
		actionReg = RTL9603D_EXTG_ACTYPE1r;
		break;
	case 2:
		actionReg = RTL9603D_EXTG_ACTYPE2r;
		break;
	case 3:
		actionReg = RTL9603D_EXTG_ACTYPE3r;
		break;
	case 4:
		actionReg = RTL9603D_EXTG_ACTYPE4r;
		break;
	case 5:
		actionReg = RTL9603D_EXTG_ACTYPE5r;
		break;
	case 6:
		actionReg = RTL9603D_EXTG_ACTYPE6r;
		break;
	case 7:
		actionReg = RTL9603D_EXTG_ACTYPE7r;
		break;
	default:
		return RT_ERR_RG_FAILED;
		break;
		
	}

	/* actionList 1~7 mapping to port(array) 0~6 */
	if ((ret = reg_array_read(actionReg, actionList-1, REG_ARRAY_INDEX_NONE, (uint32 *)pExtraTagAction)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Extra tag list %d, act index %d, act bit = %d", actionList, actionIdx, pExtraTagAction->type1.act_bit);

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagContentBuffer_set(uint32 actionList, char *pContentBuffer)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, i, offset = 0, len = 512;
	rtk_rg_asic_extraTagAction_t extraTagAct;
	uint32 *pDataBuf = (uint32 *)pContentBuffer;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "for actList %d", actionList);
		
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((pContentBuffer == NULL), RT_ERR_RG_NULL_POINTER);


	for(i=0; i<RTL9603D_EXTRATAG_ACTIONS; i++)
	{
		rtk_rg_asic_extraTagAction_get(actionList, i, &extraTagAct);
		
		if(extraTagAct.type1.act_bit == FB_EXTG_ACTBIT_NOACTION)
			break;
		
		if(extraTagAct.type1.act_bit == FB_EXTG_ACTBIT_1)
		{
			offset = extraTagAct.type1.src_addr_offset;
			len = extraTagAct.type1.length;
		}
		
		if(extraTagAct.type3.act_bit == FB_EXTG_ACTBIT_3)
		{
			for(tmpVal = 0; tmpVal < extraTagAct.type3.length; tmpVal++)
				pContentBuffer[extraTagAct.type3.pkt_buff_offset+tmpVal] = 0;	
		}
		
		if(extraTagAct.type4.act_bit == FB_EXTG_ACTBIT_4)
		{
			if(extraTagAct.type4.data_src_type==0)
			{	// GRE seq/ack
				int greOffset = 0;
				if(!extraTagAct.type4.reduce_seq)
				{
					for(tmpVal = 0; tmpVal < 4; tmpVal++)
						pContentBuffer[extraTagAct.type4.pkt_buff_offset+tmpVal] = 0;	
					greOffset += 4;
				}
				if(!extraTagAct.type4.reduce_ack)
				{
					for(tmpVal = 0; tmpVal < 4; tmpVal++)
						pContentBuffer[extraTagAct.type4.pkt_buff_offset+greOffset+tmpVal] = 0;	
				}
					
			}else if (extraTagAct.type4.data_src_type==1)
			{	//IPv4ID
				for(tmpVal = 0; tmpVal < 2; tmpVal++)
					pContentBuffer[extraTagAct.type4.pkt_buff_offset+tmpVal] = 0;	
			}
		}
		
		if(extraTagAct.type5.act_bit == FB_EXTG_ACTBIT_5)
		{
			rgip_t *ip = (rgip_t *)pContentBuffer;
			ip->ip_sum = _rtk_rg_outerHdrIPv4Checksum(ip);
		}
		
		if(extraTagAct.type6.act_bit == FB_EXTG_ACTBIT_6)
		{
			rgip_t *ip = (rgip_t *)pContentBuffer;
			rgudpHdr_t *udph = (rgudpHdr_t *)(pContentBuffer + (ip->ip_vhl&0xf)*4);	// ipv4 header length
			udph->uh_sum = _rtk_rg_outerHdrUdpChecksum(ip, udph, len-sizeof(rgip_t)/*UDP Header Length*/);	 // IPv4(20)+UDP(8)+L2TP(6~8)+PPP(1~4) - IPHdrLen = UDPLen
		}
	}

	// write data register per word
	offset = offset>>2;
	if(len&0x3)
		len = (len>>2)+1;
	else
		len = len>>2;
	
	for(i = 0; i < len; i++){
		tmpVal = pDataBuf[i];
		if ((ret = reg_array_write(RTL9603D_EXTHDR_DATr, offset+i, REG_ARRAY_INDEX_NONE, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagContentBuffer_get(uint32 offset, uint32 len, char *pContentBuffer)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, i;
	uint32 *pDataBuf = (uint32 *)pContentBuffer;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "offset %d, len %d", offset, len);
		
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK(((offset&0x3) != 0), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((pContentBuffer == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK(((offset+len) > 512), RT_ERR_RG_INVALID_PARAM);

	// read data register per word
	offset = offset>>2;
	if(len&0x3)
		len = (len>>2)+1;
	else
		len = len>>2;
	
	for(i = 0; i < len; i++){
		if ((ret = reg_array_read(RTL9603D_EXTHDR_DATr, offset+i, REG_ARRAY_INDEX_NONE, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
		    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
		    	return RT_ERR_FAILED;
		}
		pDataBuf[i] = tmpVal;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagInsertHdrLen_set(uint32 actionList, uint32 len)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "actionList %d, len %d", actionList, len);
		
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((actionList<RTL9603D_EXTRATAG_LISTMIN) || (actionList>RTL9603D_EXTRATAG_LISTMAX), RT_ERR_RG_INVALID_PARAM);

	tmpVal = len;
	if ((ret = reg_array_field_write(RTL9603D_EXTRA_TAG_INFOr, REG_ARRAY_INDEX_NONE, actionList-1, RTL9603D_EX_TAG_INC_LENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_extraTagInsertHdrLen_get(uint32 actionList, uint32 *len)
{		
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "actionList %d, len %d", actionList, len);
		
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((actionList<RTL9603D_EXTRATAG_LISTMIN) || (actionList>RTL9603D_EXTRATAG_LISTMAX), RT_ERR_RG_INVALID_PARAM);

	if ((ret = reg_array_field_read(RTL9603D_EXTRA_TAG_INFOr, REG_ARRAY_INDEX_NONE, actionList-1, RTL9603D_EX_TAG_INC_LENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}	
	*len = tmpVal;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowTrfIndicator_get(uint32 idx, rtk_enable_t *pFlowIndicator)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	uint32 posiIdx;
		
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pFlowIndicator), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((idx >= _rtk_rg_flowEntryNum_get()), RT_ERR_RG_INVALID_PARAM);

	posiIdx = idx>>5;
	
	if ((ret = reg_array_field_read(RTL9603D_FT_TRFr, REG_ARRAY_INDEX_NONE, posiIdx, RTL9603D_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	*pFlowIndicator = (tmpVal & (1 <<(idx&31)))? ENABLED : DISABLED;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "flow %d trf indicator=%d", idx, *pFlowIndicator);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowTraffic_get(uint32 *validSet, uint32 *flowTrafficSet)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	uint32 i, entryNum;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "get flow traffic bits");
	
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == validSet), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((NULL == flowTrafficSet), RT_ERR_RG_NULL_POINTER);

	entryNum = _rtk_rg_flowEntryNum_get() >> 5;	
	
	/* Get Traffic Table(Register) */
	for(i = 0; i < entryNum; i++)
	{
		if(validSet[i])
		{
			if ((ret = reg_array_read(RTL9603D_FT_TRFr, REG_ARRAY_INDEX_NONE, i, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			    	return ret;
			}
			flowTrafficSet[i] = tmpVal;
		}else
			flowTrafficSet[i] = 0;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifTrfIndicator_get(uint32 idx, rtk_enable_t *pNetifIndicator)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=idx), RT_ERR_ENTRY_INDEX);
	RT_PARAM_CHK((NULL == pNetifIndicator), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_IF_TRFr, RTL9603D_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	*pNetifIndicator = (tmpVal & (1<<idx)) ? ENABLED : DISABLED;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "netif %d trf indicator=%d", idx, *pNetifIndicator);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_netifTraffic_get(uint32 *value)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == value), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_IF_TRFr, RTL9603D_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	*value = tmpVal & 0xffff;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "netiftrf bits=0x%x", *value);
	
	return RT_ERR_RG_OK;
}


/* Function Name:
 *      rtk_rg_asic_lutSATraffic_get
 * Description:
 *      Get LUT SA traffic bits.
 * Input:
 *      lutStartsIdx	- starts index (lutOffset) of lut, MUST BE a multiple of 32.
 * Output:
 *      pTrfBitMask	- the traffic status (32bits) from lutOffset to lutOffset+31. 
 * Return:
 *      RT_ERR_RG_OK                 	- OK
 *      RT_ERR_OUT_OF_RANGE	- input parameter out of range
 *      RT_ERR_NULL_POINTER	- input parameter is null pointer
 * Note:
 *      NA
 */
rtk_rg_err_code_t rtk_rg_asic_lutSATraffic_get(uint32 lutStartsIdx, uint32 *pTrfBitMask)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK(((LUTTABLE_SRAM_SIZE+LUTTABLE_BCAM_SIZE) <= lutStartsIdx), RT_ERR_OUT_OF_RANGE);
	RT_PARAM_CHK((NULL == pTrfBitMask), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK(((lutStartsIdx&0x1f)!=0), RT_ERR_RG_INVALID_PARAM);

	if ((ret = reg_array_read(RTL9603D_LUT_SA_TRFr, REG_ARRAY_INDEX_NONE, lutStartsIdx>>5, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	*pTrfBitMask = tmpVal;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "LUT index starts from %d - traffic bits=0x%x", lutStartsIdx, *pTrfBitMask);
	
	return RT_ERR_RG_OK;

}

extern rtk_rg_err_code_t rtk_rg_asic_lutSATraffic_reset(void)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 i = 0, tmpVal = 0;

	for(i = 0; i < (LUTTABLE_SRAM_SIZE+LUTTABLE_BCAM_SIZE); i+=32)
	{
		ret = rtk_rg_asic_lutSATraffic_get(i, &tmpVal);
		if(ret!=RT_ERR_RG_OK) RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	}

	return ret;
}

rtk_rg_err_code_t rtk_rg_asic_unmatchedCpuPriority_set(uint32 priority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "priority=%d", priority);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK(((1<<3) <= priority), RT_ERR_INPUT);

	tmpVal = priority;
	if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_UNMATCHED_CPU_PRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_unmatchedCpuPriority_get(uint32 *pPriority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pPriority), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_UNMATCHED_CPU_PRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	*pPriority = tmpVal & 0x7;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "priority=%d", *pPriority);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_trapCpuPriority_set(rtk_enable_t state, uint32 priority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "state=%d,priority=%d", state, priority);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK(((1<<3) <= priority), RT_ERR_INPUT);
	RT_PARAM_CHK((state!=DISABLED)&&(state!=ENABLED), RT_ERR_INPUT);
	
	tmpVal = state;
	if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_FORCE_TRAP_CPU_PRI_ENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	tmpVal = priority;
	if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_FORCE_TRAP_CPU_PRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_trapCpuPriority_get(rtk_enable_t *pState, uint32 *pPriority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;


	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pState), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((NULL == pPriority), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_read(RTL9603D_LUPr, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	*pState = (tmpVal & 0x8) ? ENABLED : DISABLED;
	*pPriority = tmpVal & 0x7;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "state=%d,priority=%d", *pState, *pPriority);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_spaUnmatchAction_set(rtk_rg_asic_forwardAction_t action)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "SPA UnmatchAct=%d", action);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((FB_ACTION_END <= action), RT_ERR_INPUT);
	RT_PARAM_CHK((FB_ACTION_FORWARD > action), RT_ERR_INPUT);
	
	tmpVal = action;
	if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_SPA_NOT_IN_INTF_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_spaUnmatchAction_get(rtk_rg_asic_forwardAction_t *pAction)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);
	
	if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_SPA_NOT_IN_INTF_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	*pAction = tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "SPA UnmatchAct=%d", *pAction);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_switchBufFullAction_set(rtk_rg_asic_forwardAction_t action)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A
	
	/* parameter check */
	RT_PARAM_CHK((action >= FB_ACTION_END), RT_ERR_RG_INVALID_PARAM);

	switch(action)
	{
		case FB_ACTION_FORWARD:
			tmpVal = 0;
			break;
		case FB_ACTION_TRAP2CPU:
			tmpVal = 1;
			break;
		case FB_ACTION_DROP:
			tmpVal = 2;
			break;
		default:	// forbidden
			return RT_ERR_RG_INVALID_PARAM;
	}

	if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_BUF_FULL_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
		
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_switchBufFullAction_get(rtk_rg_asic_forwardAction_t *pAction)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((pAction == NULL), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_BUF_FULL_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	switch(tmpVal)
	{
		case 0:
			*pAction = FB_ACTION_FORWARD;
			break;
		case 1:
			*pAction = FB_ACTION_TRAP2CPU;
			break;
		case 2:
			*pAction = FB_ACTION_DROP;
			break;
		default:	// forbidden
			*pAction = FB_ACTION_END;
			return RT_ERR_RG_INVALID_PARAM;
	}
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_switchBufFullThreshold_set(uint32 pageSize)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	RT_PARAM_CHK((pageSize >= (1<<10)), RT_ERR_RG_INVALID_PARAM);

	if ((ret = reg_field_write(RTL9603D_FC_FB_P_EGR_DROP_THr, RTL9603D_THf, (uint32 *)&pageSize)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_switchBufFullThreshold_get(uint32 *pPageSize)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	
	RT_PARAM_CHK((pPageSize == NULL), RT_ERR_RG_NULL_POINTER);
	
	if ((ret = reg_field_read(RTL9603D_TCP_ACKr, RTL9603D_TCP_ACK_PRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	*pPageSize = tmpVal & 0x3ff;
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_tcpAckPriority_set(uint32 priority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	/* check Init status */
	// N/A
	
	/* parameter check */
	RT_PARAM_CHK((priority >= 8), RT_ERR_RG_INVALID_PARAM);

	if ((ret = reg_field_write(RTL9603D_TCP_ACKr, RTL9603D_TCP_ACK_PRIf, (uint32 *)&priority)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
		
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_tcpAckPriority_get(uint32 *pPriority)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((pPriority == NULL), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_TCP_ACKr, RTL9603D_TCP_ACK_PRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	*pPriority = tmpVal & 0x7;

	return RT_ERR_RG_OK;
}



extern rtk_rg_err_code_t rtk_rg_asic_tcpAckPort_set(rtk_rg_port_idx_t portIdx , rtk_enable_t state)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal = 0;
	
	/*input error check*/
	RT_PARAM_CHK(RG_INVALID_PORT(portIdx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((RTK_ENABLE_END<=state), RT_ERR_RG_INVALID_PARAM);

	/* check Init status */
	// N/A

	if ((ret = reg_field_read(RTL9603D_TCP_ACKr, RTL9603D_SPA_MASKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
    	{
        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
        	return ret;
	}

	if(state == ENABLED)
		tmpVal |= (1<<portIdx);
	else
		tmpVal &= ~(1<<portIdx);

	if ((ret = reg_field_write(RTL9603D_TCP_ACKr, RTL9603D_SPA_MASKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
    	{
        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
        	return ret;
	}
	
	
	return RT_ERR_RG_OK;
}

extern rtk_rg_err_code_t rtk_rg_asic_tcpAckPort_get(rtk_rg_port_idx_t portIdx , rtk_enable_t *pState)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal = 0;
	
	/*input error check*/
	RT_PARAM_CHK(RG_INVALID_PORT(portIdx), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((pState==NULL), RT_ERR_RG_NULL_POINTER);

	/* check Init status */
	// N/A

	if ((ret = reg_field_read(RTL9603D_TCP_ACKr, RTL9603D_SPA_MASKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
    	{
        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
        	return ret;
	}
	*pState = (tmpVal >> portIdx) & 0x1;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_globalState_set(rtk_rg_asic_globalStateType_t stateType, rtk_enable_t state)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "stateType=%d,state=%d",stateType, state);

	/* check Init status */
	//NA

	/* parameter check */
	RT_PARAM_CHK((FB_GLOBAL_STATE_END <=stateType), RT_ERR_INPUT);
	RT_PARAM_CHK((RTK_ENABLE_END <= state), RT_ERR_INPUT);

	switch(stateType){
	case FB_GLOBAL_TTL_1:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_TTL_1f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_GLOBAL_TRAP_TCP_SYN_FIN_REST:
		tmpVal = state & 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_TRAP_TCP_FLAGf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
		}
	break;
	case FB_GLOBAL_TRAP_TCP_SYN_ACK:
		tmpVal = state & 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_TRAP_SYC_ACKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_TRAP_FRAGMENT:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_TRAP_FRAGMENTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_L3_CS_CHK:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_L3_CS_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_L4_CS_CHK:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_L4_CS_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH12_SKIP_CVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH12_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_CVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_CVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH5_SKIP_CVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH5_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH6_SKIP_CVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH6_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH12_SKIP_SVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH12_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_SVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_SVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH5_SKIP_SVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH5_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH6_SKIP_SVID:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH6_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_SKIP_DA:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_SKIP_DAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH12_SKIP_CPRI:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH12_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_CPRI:	
		tmpVal = (state == DISABLED) ? 0 : 1;	
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_CPRI:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH5_SKIP_CPRI:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH5_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH6_SKIP_CPRI:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH6_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATHALL_SKIP_DSCP:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_SKIP_DSCPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH6_SKIP_DA:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH6_SKIP_DAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH6_SKIP_SA:		
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_PATH6_SKIP_SAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_L2_FLOW_LOOKUP_BY_MAC:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_L2F_LUP_BY_MACf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_CMP_TOS:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_L34r, RTL9603D_CMP_TOSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_TRAP_INSERTTAG_PADDING:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_TRAP_PAD_PKTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_PATH5_ALOW_PURE_L3:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_P5_ALOW_PURE_L3f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	case FB_GLOBAL_UCBRIDGE_FRAG_FORCE_P12:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_UC_BRD_FRG_P12f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	break;
	default:
		return RT_ERR_INPUT;
	break;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_globalState_get(rtk_rg_asic_globalStateType_t stateType, rtk_enable_t *pState)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "stateType=%d",stateType);

	/* check Init status */
	//NA

	/* parameter check */
	RT_PARAM_CHK((FB_GLOBAL_STATE_END <=stateType), RT_ERR_INPUT);
	RT_PARAM_CHK((NULL == pState), RT_ERR_RG_NULL_POINTER);

	switch(stateType){
	case FB_GLOBAL_TTL_1:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_TTL_1f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	 RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
	    	*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_TRAP_TCP_SYN_FIN_REST:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_TRAP_TCP_FLAGf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
		}
		*pState = tmpVal & 1;
	break;
	case FB_GLOBAL_TRAP_TCP_SYN_ACK:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_TRAP_SYC_ACKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	        	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = tmpVal & 1;
	break;
	case FB_GLOBAL_TRAP_FRAGMENT:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_TRAP_FRAGMENTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_L3_CS_CHK:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_L3_CS_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_L4_CS_CHK:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_L4_CS_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH12_SKIP_CVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH12_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_CVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_CVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH5_SKIP_CVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH5_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH6_SKIP_CVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH6_SKIP_CVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH12_SKIP_SVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH12_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_SVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_SVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH5_SKIP_SVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH5_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH6_SKIP_SVID:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH6_SKIP_SVIDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_SKIP_DA:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_SKIP_DAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH12_SKIP_CPRI:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH12_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_UCBC_SKIP_CPRI:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_UC_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH34_MC_SKIP_CPRI:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH34_MC_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH5_SKIP_CPRI:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH5_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH6_SKIP_CPRI:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH6_SKIP_CPRIf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATHALL_SKIP_DSCP:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_SKIP_DSCPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH6_SKIP_DA:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH6_SKIP_DAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH6_SKIP_SA:		
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_PATH6_SKIP_SAf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_L2_FLOW_LOOKUP_BY_MAC:
		if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_L2F_LUP_BY_MACf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_CMP_TOS:
		if ((ret = reg_field_read(RTL9603D_L34r, RTL9603D_CMP_TOSf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_TRAP_INSERTTAG_PADDING:
		if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_TRAP_PAD_PKTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_PATH5_ALOW_PURE_L3:
		if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_P5_ALOW_PURE_L3f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	case FB_GLOBAL_UCBRIDGE_FRAG_FORCE_P12:
		if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_UC_BRD_FRG_P12f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	    	{
	       	RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	        	return ret;
	    	}
		*pState = (tmpVal == 0) ? DISABLED : ENABLED;
	break;
	default:
		return RT_ERR_INPUT;
	break;
	}

	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_fbModeCtrl_set(rtk_rg_asic_fbModeCtrl_t ctrlType, uint8 state)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ctrlType=%d", ctrlType);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((FB_MODE_FB_MOD < ctrlType), RT_ERR_INPUT);
	
	if((ctrlType == FB_MODE_FB_MOD)){
		if(state > FB_MODE_32K)
			return RT_ERR_RG_INVALID_PARAM;
	}else{
		/* For other control register, only support 0/1 */
		if(state > 1)
			return RT_ERR_RG_INVALID_PARAM;
	}		
	
	switch(ctrlType)
	{
		case FB_MODE_RST_VLD:
		case FB_MODE_RST_TRF:
			if(state == 1){
				uint32 tableField;
				if(ctrlType == FB_MODE_RST_VLD)
					tableField = RTL9603D_RST_VLDf;
				else
					tableField = RTL9603D_RST_TRFf;
					
				tmpVal = 0x1;
				if ((ret = reg_field_write(RTL9603D_MODEr, tableField, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
				{
					RT_ERR(ret, (MOD_L34|MOD_DAL), "");
					return ret;
				}
				
				/* Wait operation completed */
				do
				{
					if ((ret = reg_field_read(RTL9603D_MODEr, tableField, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
					{
						return ret;
					}
				} while (tmpVal);
			}
			break;
		case FB_MODE_FB_MOD:
			tmpVal = state & 0x3;
			if ((ret = reg_field_write(RTL9603D_MODEr, RTL9603D_FB_MODf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return ret;
			}
			break;	
		default:
			return RT_ERR_INPUT;
			break;
				
	}
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_fbModeCtrl_get(rtk_rg_asic_fbModeCtrl_t ctrlType, uint8 *pState)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ctrlType=%d", ctrlType);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((FB_MODE_FB_MOD < ctrlType), RT_ERR_INPUT);
	RT_PARAM_CHK((NULL == pState), RT_ERR_RG_NULL_POINTER);
	
	switch(ctrlType)
	{
	case FB_MODE_RST_VLD:
		if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_RST_VLDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_MODE_RST_TRF:
		if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_RST_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_MODE_FB_MOD:
		if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_FB_MODf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;	
	default:
		return RT_ERR_INPUT;
		break;
				
	}

	*pState = (tmpVal & 0xff);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_preHashPtn_set(rtk_rg_asic_preHashPtn_t ptnType, uint32 code)
{	
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ptnType=%d",ptnType);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((FB_PREHASH_PTN_END <= ptnType), RT_ERR_INPUT);
	
	switch(ptnType)
	{
		case FB_PREHASH_PTN_SPORT:		
			tmpVal = code & 0xfffff;
			if ((ret = reg_field_write(RTL9603D_PRE_HASH_ITM1r, RTL9603D_PTN_SPORTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return ret;
			}
			break;
		case FB_PREHASH_PTN_DPORT:
			tmpVal = code & 0xfffff;
			if ((ret = reg_field_write(RTL9603D_PRE_HASH_ITM2r, RTL9603D_PTN_DPORTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return ret;
			}
			break;
		case FB_PREHASH_PTN_SIP:
			tmpVal = code & 0xffffff;
			if ((ret = reg_field_write(RTL9603D_PRE_HASH_ITM3r, RTL9603D_PTN_SIPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return ret;
			}
			break;
		case FB_PREHASH_PTN_DIP:
			tmpVal = code & 0xffffff;
			if ((ret = reg_field_write(RTL9603D_PRE_HASH_ITM4r, RTL9603D_PTN_DIPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				RT_ERR(ret, (MOD_L34|MOD_DAL), "");
				return ret;
			}
			break;	
		default:
			return RT_ERR_INPUT;
			break;
	}
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_preHashPtn_get(rtk_rg_asic_preHashPtn_t ptnType, uint32 *pCode)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "ptnType=%d",ptnType);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((FB_PREHASH_PTN_END <= ptnType), RT_ERR_INPUT);
	RT_PARAM_CHK((NULL == pCode), RT_ERR_RG_NULL_POINTER);
	
	switch(ptnType)
	{
	case FB_PREHASH_PTN_SPORT:
		if ((ret = reg_field_read(RTL9603D_PRE_HASH_ITM1r, RTL9603D_PTN_SPORTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_PREHASH_PTN_DPORT:
		if ((ret = reg_field_read(RTL9603D_PRE_HASH_ITM2r, RTL9603D_PTN_DPORTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_PREHASH_PTN_SIP:
		if ((ret = reg_field_read(RTL9603D_PRE_HASH_ITM3r, RTL9603D_PTN_SIPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_PREHASH_PTN_DIP:
		if ((ret = reg_field_read(RTL9603D_PRE_HASH_ITM4r, RTL9603D_PTN_DIPf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;	
	default:
		return RT_ERR_INPUT;
		break;
	}

	*pCode = tmpVal;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_dualHdrInfo_set(rtk_rg_asic_dualHdrInfo_t target, uint8 intfIdx, uint32 value)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Target type =%d", target);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX()<=intfIdx), RT_ERR_ENTRY_INDEX);
	
	switch(target)
	{
	case FB_DUALHDR_GRESEQ:
		if ((ret = reg_array_write(RTL9603D_GRE_SEQr, REG_ARRAY_INDEX_NONE, intfIdx, (uint32 *)&value)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_DUALHDR_GREACK:
		if ((ret = reg_array_write(RTL9603D_GRE_ACKr, REG_ARRAY_INDEX_NONE, intfIdx, (uint32 *)&value)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_DUALHDR_OUTER_IPV4ID:
		if ((ret = reg_array_field_write(RTL9603D_FB_IPID_TBLr, REG_ARRAY_INDEX_NONE, intfIdx, RTL9603D_IP4_IDf, (uint32 *)&value)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	default:
		WARNING("target %d is not supported", target);
		return RT_ERR_INPUT;
		break;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_dualHdrInfo_get(rtk_rg_asic_dualHdrInfo_t target, uint8 intfIdx, uint32 *value)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Target type =%d", target);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((HAL_L34_NETIF_ENTRY_MAX() <= intfIdx), RT_ERR_INPUT);
	RT_PARAM_CHK((NULL == value), RT_ERR_RG_NULL_POINTER);
	
	switch(target)
	{
	case FB_DUALHDR_GRESEQ:
		if ((ret = reg_array_read(RTL9603D_GRE_SEQr, REG_ARRAY_INDEX_NONE, intfIdx, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_DUALHDR_GREACK:
		if ((ret = reg_array_read(RTL9603D_GRE_ACKr, REG_ARRAY_INDEX_NONE, intfIdx, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	case FB_DUALHDR_OUTER_IPV4ID:
		if ((ret = reg_array_field_read(RTL9603D_FB_IPID_TBLr, REG_ARRAY_INDEX_NONE, intfIdx, RTL9603D_IP4_IDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	default:
		WARNING("target %d is not supported", target);
		return RT_ERR_INPUT;
		break;
	}

	*value = tmpVal;
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_shareMeter_set(uint32 index, uint32 rate, rtk_enable_t ifgInclude)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34),"meter %d, rate %d, ifgInclude", index, rate, ifgInclude);

	/* check Init status */
	RT_INIT_CHK(fb_init);


	/* parameter check */
	RT_PARAM_CHK((RTL9603D_TABLESIZE_SHAREMTR <= index), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK(((RTL9603D_METER_RATE_MAX) < rate), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((RTK_ENABLE_END <= ifgInclude), RT_ERR_RG_NULL_POINTER);

	/*set IFG*/
	tmpVal = (ifgInclude==ENABLED) ? 1 : 0;
	if (( ret = reg_array_field_write(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_IFGf, &tmpVal)) != RT_ERR_RG_OK )
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	/*set rate*/
	tmpVal = rate>>3;
	if (( ret = reg_array_field_write(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_RATEf, &tmpVal)) != RT_ERR_RG_OK )
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_shareMeter_get(uint32 index, uint32 *pRate , rtk_enable_t *pIfgInclude)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((RTL9603D_TABLESIZE_SHAREMTR <= index), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((NULL == pRate), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((NULL == pIfgInclude), RT_ERR_RG_NULL_POINTER);

	/*get include IFG status*/
	if (( ret = reg_array_field_read(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_IFGf, &tmpVal)) != RT_ERR_RG_OK )
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pIfgInclude = (tmpVal == ENABLED) ? 1 : 0;

	/*get rate setting*/
	if (( ret = reg_array_field_read(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_RATEf, &tmpVal)) != RT_ERR_RG_OK )
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pRate = tmpVal*8;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34),"meter %d, rate %d, ifgInclude", index, *pRate, *pIfgInclude);

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_shareMeterBucket_set(uint32 index, uint32 bucketSize)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34),"meter %d, bucketSize %d", index, bucketSize);

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((RTL9603D_TABLESIZE_SHAREMTR <= index), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((RTL9603D_METER_BUCKETSIZE_MAX < bucketSize), RT_ERR_RG_INVALID_PARAM);

	tmpVal = bucketSize;
	if ((ret = reg_array_field_write(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_LBHTf, &tmpVal)) != RT_ERR_RG_OK )
	{
	    RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    return ret;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_shareMeterBucket_get(uint32 index, uint32 *pBucketSize)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((RTL9603D_TABLESIZE_SHAREMTR <= index), RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((NULL == pBucketSize), RT_ERR_RG_NULL_POINTER);

	if (( ret = reg_array_field_read(RTL9603D_MTR_SETTINGr, REG_ARRAY_INDEX_NONE, index, RTL9603D_METER_LBHTf, &tmpVal)) != RT_ERR_RG_OK )
	{
	    RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    return ret;
	}
	*pBucketSize = tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34),"meter %d, bucketSize %d", index, *pBucketSize);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_shareMeterGlobalConfig_set(uint32 tickNum, uint32 decCnt)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((255 < tickNum), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((255 < decCnt), RT_ERR_RG_INVALID_PARAM);

	tmpVal = tickNum;
	if ((ret = reg_field_write(RTL9603D_MTR_CTRLr, RTL9603D_MTR_TICKf, &tmpVal)) != RT_ERR_RG_OK )
	{
	    RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    return ret;
	}
	
	tmpVal = decCnt;
	if ((ret = reg_field_write(RTL9603D_MTR_CTRLr, RTL9603D_DEC_CNTf, &tmpVal)) != RT_ERR_RG_OK )
	{
	    RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    return ret;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_shareMeterTick_get(uint32 *pTickNum)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pTickNum), RT_ERR_RG_NULL_POINTER);

	if (( ret = reg_field_read(RTL9603D_MTR_CTRLr, RTL9603D_MTR_TICKf, &tmpVal)) != RT_ERR_RG_OK )
	{
	    RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    return ret;
	}
	*pTickNum = tmpVal;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_table_reset(rtk_rg_asic_resetTableType_t type)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, retry;
	uint32 tableReg = RTL9603D_NAT_TBL_ACCESS_CLRr;
	uint32 tableField = 0;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Reset table =%d", type);

	if(type == FB_RST_ALL){
		tmpVal = 0xffff;
		if ((ret = reg_write(RTL9603D_NAT_TBL_ACCESS_CLRr, (uint32 *)&tmpVal)) != RT_ERR_RG_OK )
		{
		    RT_ERR(ret, (MOD_L2|MOD_DAL), "");
		    return ret;
		}
		
		/* Wait operation completed */
		retry = 0;
		do
		{
			if ((ret = reg_read(RTL9603D_NAT_TBL_ACCESS_CLRr, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				return ret;
			}
			retry++;
		}while ((1 == tmpVal) && (retry < RETRY_TO_SHORT));
		if(retry >= RETRY_TO_SHORT)	return RT_ERR_BUSYWAIT_TIMEOUT;

		tmpVal = 0x1;
		if ((ret = reg_field_write(RTL9603D_MODEr, RTL9603D_RST_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		
		/* Wait operation completed */
		retry = 0;
		do
		{
			if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_RST_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				return ret;
			}
			retry++;
		} while ((1 == tmpVal) && (retry < RETRY_TO_SHORT));
		if(retry >= RETRY_TO_SHORT)	return RT_ERR_BUSYWAIT_TIMEOUT;

		tmpVal = 0x1;
		if ((ret = reg_field_write(RTL9603D_MODEr, RTL9603D_RST_VLDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		
		/* Wait operation completed */
		retry = 0;
		do
		{
			if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_RST_VLDf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				return ret;
			}
			retry++;
		} while ((1 == tmpVal) && (retry < RETRY_TO_SHORT));
		if(retry >= RETRY_TO_SHORT)	return RT_ERR_BUSYWAIT_TIMEOUT;
		
		_rtk_rg_flowEntryValidBit_reset(FB_flowEtnryScope_ALL);
		for(tmpVal = 0; tmpVal < HAL_L34_NETIF_ENTRY_MAX(); tmpVal++)
		{
			ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_GRESEQ, tmpVal, 0), SUCCESS);
			ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_GREACK, tmpVal, 0), SUCCESS);
			ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_OUTER_IPV4ID, tmpVal, 0), SUCCESS);
		}
		
		for(tmpVal = 0; tmpVal < RTL9603D_TABLESIZE_EXTPORT; tmpVal++)
		{
			ASSERT_EQ(rtk_rg_asic_extPortMaskTable_del(tmpVal), SUCCESS);
		}
	}else{
		switch(type)
		{
		case FB_RST_IF_TBL:
			tableField = RTL9603D_RST_IF_TBLf;
			for(tmpVal = 0; tmpVal < HAL_L34_NETIF_ENTRY_MAX(); tmpVal++)
			{
				ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_GRESEQ, tmpVal, 0), SUCCESS);
				ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_GREACK, tmpVal, 0), SUCCESS);
				ASSERT_EQ(rtk_rg_asic_dualHdrInfo_set(FB_DUALHDR_OUTER_IPV4ID, tmpVal, 0), SUCCESS);
			}
			break;
		case FB_RST_ETHER_TYPE:
			tableField = RTL9603D_RST_ETHER_TYPEf;
			break;
		case FB_RST_EXT_PMASK_TYPE:
			tableField = RTL9603D_RST_EXT_PORTf;
			for(tmpVal = 0; tmpVal < RTL9603D_TABLESIZE_EXTPORT; tmpVal++)
			{
				ASSERT_EQ(rtk_rg_asic_extPortMaskTable_del(tmpVal), SUCCESS);
			}
			break;
		case FB_RST_WAL_TYPE:
			TABLE("table is removed");
			return RT_ERR_OK;
			break;
		case FB_RST_FLOW_TBL:
			tableField = RTL9603D_RST_FLOW_TBLf;
			_rtk_rg_flowEntryValidBit_reset(FB_flowEtnryScope_SRAM);
			break;
		case FB_RST_CAM_TBL:
			tableField = RTL9603D_RST_CAM_TBLf;
			break;
		case FB_RST_MAC_INDEX_TBL:
			tableField = RTL9603D_RST_MAC_INDEX_TBLf;
			break;
		case FB_RST_TCAM_TBL:
			tableField = RTL9603D_RST_TCAM_TBLf;
			_rtk_rg_flowEntryValidBit_reset(FB_flowEtnryScope_TCAM);
			break;
	    	case FB_RST_TRAFFIC_TBL:	
			tableReg = RTL9603D_MODEr;
			tableField = RTL9603D_RST_TRFf;
	        	break;
	    	case FB_RST_VALID_TBL:	
			tableReg = RTL9603D_MODEr;
			tableField = RTL9603D_RST_VLDf;
	        	break;
		default:
			WARNING("reset table type %d is not supported", type);
			return RT_ERR_INPUT;
			break;
		}

		tmpVal = 0x1;
		if ((ret = reg_field_write(tableReg, tableField, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		
		/* Wait operation completed */
		retry = 0;
		do
		{
			if ((ret = reg_field_read(tableReg, tableField, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
			{
				return ret;
			}
			retry++;
		} while ((1 == tmpVal) && (retry < RETRY_TO_SHORT));
		if(retry >= RETRY_TO_SHORT)	return RT_ERR_BUSYWAIT_TIMEOUT;
	}
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccMemAddr_set(void **pMemBase)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;
	uint32 memAddr = (*pMemBase==NULL) ? 0 : (long)*pMemBase;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "memory addr = 0x%x", memAddr);

	/* parameter check */
	RT_PARAM_CHK(((memAddr%1024) != 0), RT_ERR_INPUT);	// MUST align 1K bytes

	fb_cc_init = (*pMemBase==NULL) ? INIT_NOT_COMPLETED : INIT_COMPLETED;

#if !defined(CONFIG_APOLLOPRO_ASICMODE)
#if defined(CONFIG_APOLLOPRO_FPGA) || defined(CONFIG_APOLLO_FPGA_PHY_TEST)
	return RT_ERR_RG_OK;
#endif
#endif
	/* function body */
	memAddr &= 0x7fffffff; // write physical address
	tmpVal = memAddr / 1024;
	if ((ret = reg_field_write(RTL9603D_CC_BABr, RTL9603D_BABf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccMemAddr_get(void **pMemBase)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	
#if defined(CONFIG_APOLLOPRO_FPGA) || defined(CONFIG_APOLLO_FPGA_PHY_TEST)
	return RT_ERR_RG_OK;
#endif
	/* function body */
	if ((ret = reg_field_read(RTL9603D_CC_BABr, RTL9603D_BABf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pMemBase = (void *)(tmpVal * 1024);

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "memory addr = %p", *pMemBase);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccInvalidFlow_cmd(rtk_enable_t invValid, uint32 entryIdx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, entryNum, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((invValid >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((entryNum=_rtk_rg_flowEntryNum_get()) <= entryIdx, RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = entryIdx;
	cmdData.cBit = 1;
	cmdData.aBit = 1;
	cmdData.vBit = (invValid == ENABLED) ? 1 : 0;
	cmdData.cmdCode = 0x02;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;

	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccAddFlow_cmd(rtk_enable_t forceAdd, rtk_enable_t highPri, rtk_enable_t rstValid, rtk_enable_t addToCAM, rtk_enable_t addToCache, uint32 entryIdx, uint32 *pFlowData)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, entryNum, i, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((forceAdd >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((highPri >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((rstValid >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((addToCAM >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((addToCache >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((entryNum=_rtk_rg_flowEntryNum_get()) <= entryIdx, RT_ERR_RG_INDEX_OUT_OF_RANGE);
	
	RT_PARAM_CHK((addToCAM == DISABLED) && (addToCache == DISABLED) && (rstValid == DISABLED), RT_ERR_RG_INVALID_PARAM);

	/* function body */

	if(pFlowData!=NULL)
	{
		/* Write added flow to SFLW_[i] (FPAT_w[i]) data register */
		for (i = 0 ; i < sizeof(rtl9603d_asic_flow_entry_t)/sizeof(int); i++)
		{
			if ((ret = reg_write((RTL9603D_CC_SFLW_0r + i), (uint32 *)&pFlowData[i])) != RT_ERR_RG_OK)
			{
				return ret;
			}
		}
	}
	
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = entryIdx;
	cmdData.cBit = (addToCache == ENABLED) ? 1 : 0;
	cmdData.aBit = (addToCAM == ENABLED) ? 1 : 0;
	cmdData.vBit = (rstValid == ENABLED) ? 1 : 0;
	cmdData.pBit = (highPri == ENABLED) ? 1 : 0;
	cmdData.fBit = (forceAdd == ENABLED) ? 1 : 0;
	cmdData.cmdCode = 0x04;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccUnlockAll_cmd(rtk_enable_t unlockCAM, rtk_enable_t unlockCache)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((unlockCAM >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((unlockCache >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.cBit = (unlockCache == ENABLED) ? 1 : 0;
	cmdData.aBit =  (unlockCAM == ENABLED) ? 1 : 0;
	cmdData.cmdCode = 0x09;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccUnlockFlow_cmd(uint32 entryIdx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, entryNum, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((entryNum=_rtk_rg_flowEntryNum_get()) <= entryIdx, RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = entryIdx;
	cmdData.cBit = 1;
	cmdData.aBit = 1;
	cmdData.cmdCode = 0x0A;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccReadFlow_cmd(uint32 entryIdx, uint32 *pFlowData, rtk_rg_asic_cacheReadSta_t *cReadSta)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, entryNum, i, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((entryNum=_rtk_rg_flowEntryNum_get()) <= entryIdx, RT_ERR_RG_INDEX_OUT_OF_RANGE);
	RT_PARAM_CHK((pFlowData == NULL), RT_ERR_RG_NULL_POINTER);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = entryIdx;
	cmdData.cBit = 1;
	cmdData.aBit = 1;
	cmdData.cmdCode = 0x0C;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	/* Get reading data from SFLW_[i] (FPAT_w[i]) data register */
	for (i = 0 ; i < sizeof(rtl9603d_asic_flow_entry_t)/sizeof(int); i++)
	{
		if ((ret = reg_read((RTL9603D_CC_SFLW_0r + i), (uint32 *)&pFlowData[i])) != RT_ERR_RG_OK)
		{
		    return ret;
		}
	}

	/* Read flow status */
	if ((ret = reg_read(RTL9603D_CC_SFLW_8r, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
    	{
       	 RT_ERR(ret, (MOD_L34|MOD_DAL), "");
        	return ret;
    	}
	cReadSta->TTL = (tmpVal & 0xf);				// bits [3:0]
	cReadSta->readFromCache = ((tmpVal>>8)&0x1);	// bit [8]
	cReadSta->readFromCam = ((tmpVal>>9)&0x1);	// bit [9]

	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccUpdateFlow_cmd(rtk_enable_t forceAdd, rtk_enable_t addToCAM, rtk_enable_t addToCache, uint32 entryIdx)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, entryNum, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((forceAdd >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((addToCAM >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((addToCache >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((entryNum=_rtk_rg_flowEntryNum_get()) <= entryIdx, RT_ERR_RG_INDEX_OUT_OF_RANGE);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = entryIdx;
	cmdData.cBit = (addToCache == ENABLED) ? 1 : 0;
	cmdData.aBit = (addToCAM == ENABLED) ? 1 : 0;
	cmdData.vBit = 1;
	cmdData.fBit = (forceAdd == ENABLED) ? 1 : 0;
	cmdData.cmdCode = 0x10;

	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_LONG));

	if(retry >= RETRY_TO_LONG)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccCheckAll_cmd(rtk_enable_t pktStatistic, rtk_enable_t tblStatistic)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_cacheCtrlCmd_t cmdData;
	uint32 tmpVal, retry;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "  ");

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((pktStatistic >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((tblStatistic >= RTK_ENABLE_END), RT_ERR_RG_INVALID_PARAM);

	/* function body */
	bzero(&cmdData, sizeof(rtk_rg_asic_cacheCtrlCmd_t));
	cmdData.hashID = (pktStatistic<< 1) | tblStatistic;
	cmdData.cBit = 1;
	cmdData.aBit = 1;
	cmdData.cmdCode = 0x14;
	
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDf, (uint32 *)&cmdData)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_CC_CMDr, RTL9603D_CMDVf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}	
	
	/*Waiting until command finish*/
	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_CC_CMDr, RTL9603D_CMDVf, &tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_DAL|MOD_L34), "");
			return ret;
		}
		retry++;
	}while((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccSysCmdState_get(rtk_rg_asic_cacheState_t *pCCState)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pCCState == NULL), RT_ERR_RG_NULL_POINTER);

	/* function body */
	if ((ret = reg_read(RTL9603D_CC_STAr, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}

	pCCState->ccCurState = (tmpVal) & 0x3;
	pCCState->ccBusy = (tmpVal >> 2) & 0x1;
	pCCState->ccFaultStatus = (tmpVal >> 3) & 0x7;
	pCCState->ccCmdRtnCode.invRtn = (tmpVal >> 8) & 0xff;
	
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccPktStatistic_get(uint32 *pCompleteMissRate, uint32 *pCacheMissRate, uint32 *pMissCycle)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pCompleteMissRate == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((pCacheMissRate == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((pMissCycle == NULL), RT_ERR_RG_NULL_POINTER);

	/* function body */
	if ((ret = reg_read(RTL9603D_CC_STS_0r, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pCompleteMissRate = (tmpVal >> 24) & 0xff;
	*pCacheMissRate = (tmpVal >> 16) & 0xff;

	if ((ret = reg_read(RTL9603D_CC_STS_1r, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pMissCycle = tmpVal & 0x7ff;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccTblStatisticValidCnt_get(uint32 *pCacheCnt, uint32 *pCamCnt)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pCacheCnt == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((pCamCnt == NULL), RT_ERR_RG_NULL_POINTER);

	/* function body */
	if ((ret = reg_read(RTL9603D_CC_STS_2r, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pCacheCnt = tmpVal & 0x1fff;
	*pCamCnt = (tmpVal >> 16) & 0x7f;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccTblStatisticLockCnt_get(uint32 *pCacheCnt, uint32 *pCamCnt)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pCacheCnt == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((pCamCnt == NULL), RT_ERR_RG_NULL_POINTER);

	/* function body */
	if ((ret = reg_read(RTL9603D_CC_STS_3r, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		return ret;
	}
	*pCacheCnt = tmpVal & 0x1fff;
	*pCamCnt = (tmpVal >> 16) & 0x7f;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccGlobalState_set(rtk_rg_asic_cacheCtrl_globalCtrlType_t stateType, rtk_enable_t state)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "stateType=%d,state=%d",stateType, state);

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((FB_CC_GLOBAL_END <=stateType), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((RTK_ENABLE_END <= state), RT_ERR_RG_INVALID_PARAM);

	tmpVal = (state == DISABLED) ? 0 : 1;
	switch(stateType){
	case FB_CC_GLOBAL_CACHE_EN:
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_CENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_CAM_EN:
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_AENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_FLOW_VALID_EN:
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_VENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_CAM_TO_CACHE_EN:
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_MOVAENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_BUS_ALIGN:
		if(tmpVal == 0) return RT_ERR_RG_CHIP_NOT_SUPPORT;
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_FBA32f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_TAG_CHK:
		tmpVal = (state == DISABLED) ? 0 : 1;
		if ((ret = reg_field_write(RTL9603D_MODEr, RTL9603D_TAG_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	default:
		return RT_ERR_INPUT;
	break;
	}

	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccGlobalState_get(rtk_rg_asic_cacheCtrl_globalCtrlType_t stateType, rtk_enable_t *pState)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((FB_CC_GLOBAL_END <=stateType), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((pState == NULL), RT_ERR_RG_NULL_POINTER);

	switch(stateType){
	case FB_CC_GLOBAL_CACHE_EN:
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_CENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_CAM_EN:
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_AENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_FLOW_VALID_EN:
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_VENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_CAM_TO_CACHE_EN:
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_MOVAENf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_BUS_ALIGN:
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_FBA32f, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	break;
	case FB_CC_GLOBAL_TAG_CHK:
		if ((ret = reg_field_read(RTL9603D_MODEr, RTL9603D_TAG_CHKf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
			return ret;
		}
		break;
	default:
		return RT_ERR_INPUT;
	break;
	}

	*pState = (tmpVal == DISABLE) ? 0 : 1;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "stateType=%d,state=%d",stateType, *pState);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccFlowTagTTLThrd_set(uint32 initValue)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "set flow tag TTL initilized value to %d", initValue);

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((15 < initValue), RT_ERR_RG_INVALID_PARAM);

	tmpVal = initValue & 0xf;
	if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_CTTLINITf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccFlowTagTTLThrd_get(uint32 *pInitValue)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((pInitValue == NULL), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_CTTLINITf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	*pInitValue = tmpVal;
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccBusTimeOutCycle_set(rtk_rg_asic_cacheCtrl_busTimeOut_type_t type, rtk_rg_asic_cacheCtrl_busTimeOut_cycle_t cycle)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A
	
	/* parameter check */
	RT_PARAM_CHK((type >= FB_CC_BUSTO_TYPE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((cycle >= FB_CC_BUSTO_CYCLE_END), RT_ERR_RG_INVALID_PARAM);

	tmpVal = cycle & 0x7;

	if(type == FB_CC_BUSTO_TYPE_GRANT){
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_TO_SCALE_BGNTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	}else if(type == FB_CC_BUSTO_TYPE_DATA){
		if ((ret = reg_field_write(RTL9603D_CC_CFGr, RTL9603D_TO_SCALE_BDATf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	}
		
	return RT_ERR_RG_OK;
	
}

rtk_rg_err_code_t rtk_rg_asic_ccBusTimeOutCycle_get(rtk_rg_asic_cacheCtrl_busTimeOut_type_t type, rtk_rg_asic_cacheCtrl_busTimeOut_cycle_t *pCycle)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A
	
	/* parameter check */
	RT_PARAM_CHK((type >= FB_CC_BUSTO_TYPE_END), RT_ERR_RG_INVALID_PARAM);
	RT_PARAM_CHK((pCycle == NULL), RT_ERR_RG_NULL_POINTER);

	if(type == FB_CC_BUSTO_TYPE_GRANT){
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_TO_SCALE_BGNTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	}else if(type == FB_CC_BUSTO_TYPE_DATA){
		if ((ret = reg_field_read(RTL9603D_CC_CFGr, RTL9603D_TO_SCALE_BDATf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			RT_ERR(ret, (MOD_L34|MOD_DAL), "");
		    	return ret;
		}
	}
	*pCycle = tmpVal & 0x7;
		
	return RT_ERR_RG_OK;
	
}


rtk_rg_err_code_t rtk_rg_asic_ccTimeoutAction_set(rtk_rg_asic_forwardAction_t action)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A
	
	/* parameter check */
	RT_PARAM_CHK((action >= FB_ACTION_END), RT_ERR_RG_INVALID_PARAM);

	switch(action)
	{
		case FB_ACTION_TRAP2CPU:
			tmpVal = 1;
			break;
		case FB_ACTION_DROP:
			tmpVal = 2;
			break;
		default:	// forbidden
			return RT_ERR_RG_INVALID_PARAM;
	}

	if ((ret = reg_field_write(RTL9603D_LUPr, RTL9603D_CC_TO_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
		
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_ccTimeoutAction_get(rtk_rg_asic_forwardAction_t *pAction)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal;

	/* check Init status */
	// N/A

	/* parameter check */
	RT_PARAM_CHK((pAction == NULL), RT_ERR_RG_NULL_POINTER);

	if ((ret = reg_field_read(RTL9603D_LUPr, RTL9603D_CC_TO_ACTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	switch(tmpVal)
	{
		case 1:
			*pAction = FB_ACTION_TRAP2CPU;
			break;
		case 2:
			*pAction = FB_ACTION_DROP;
			break;
		default:	// forbidden
			*pAction = FB_ACTION_END;
			return RT_ERR_RG_INVALID_PARAM;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowValidBit_get(uint32 idx, rtk_enable_t *pFlowValidBit)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	rtk_rg_asic_fbMode_t fbMode = _rtk_rg_fbMode_get();
	rtk_enable_t validTblEn = DISABLED;
	uint32 posiIdx, tmpVal;
	
	/* check Init status */
	RT_INIT_CHK(fb_init);

	/* parameter check */
	RT_PARAM_CHK((NULL == pFlowValidBit), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((idx >= _rtk_rg_flowEntryNum_get()), RT_ERR_RG_INVALID_PARAM);
	
	rtk_rg_asic_ccGlobalState_get(FB_CC_GLOBAL_FLOW_VALID_EN, &validTblEn);

	if(((fbMode == FB_MODE_4K) ||(fbMode == FB_MODE_32K)) || (!validTblEn))
	{
		*pFlowValidBit = ENABLED;
		return RT_ERR_RG_OK;
	}

	// Only support in 8K/16K mode

	posiIdx = (512 + (idx>>5));
	if ((ret = reg_array_field_read(RTL9603D_FT_TRFr, REG_ARRAY_INDEX_NONE, posiIdx, RTL9603D_TRFf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	*pFlowValidBit = (tmpVal & (1 <<(idx&31)))? ENABLED : DISABLED;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "flow %d valid bit =%d", idx, *pFlowValidBit);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowTagTable_set(uint32 index, rtk_rg_asic_flowTag_entry_t *pTagEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "[FlowTagTable] Set index [%d]: msb %d TTL %d", index, pTagEntry->hashIdxMsb, pTagEntry->TTL);
	
	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pTagEntry == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((index >= RTL9603D_TABLESIZE_FLOWTAG), RT_ERR_RG_INDEX_OUT_OF_RANGE);	
	
	if ((ret = rg_asic_table_write(RTL9603D_FLOW_TABLE_TAGt, index, (uint32 *)pTagEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_flowTagTable_get(uint32 index, rtk_rg_asic_flowTag_entry_t *pTagEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((pTagEntry == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((index >= RTL9603D_TABLESIZE_FLOWTAG), RT_ERR_RG_INDEX_OUT_OF_RANGE);	
		
	if ((ret = rg_asic_table_read(RTL9603D_FLOW_TABLE_TAGt, index, (uint32 *)pTagEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "[FlowTagTable] Get index [%d]: msb %d TTL %d", index, pTagEntry->hashIdxMsb, pTagEntry->TTL);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_camTraffic_get(uint64 *pCamTrfBits)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 i, tmpVal;
	
	/* check Init status */
	
	/* parameter check */
	RT_PARAM_CHK((pCamTrfBits == NULL), RT_ERR_RG_NULL_POINTER);

	*pCamTrfBits = 0;

	if ((ret = reg_read(RTL9603D_CAM_TRFr, (uint32 *)&tmpVal)) != RT_ERR_RG_OK) {
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}
	
	*pCamTrfBits |= tmpVal;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "CAM traffic bits= 0x%llx", *pCamTrfBits);
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_camTagTable_set(uint32 index, rtk_rg_asic_camTag_entry_t *pTagEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;

	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "[CamTagTable] Set index [%d]: hashid %d lock %d valid %d", index, pTagEntry->hsahIdx, pTagEntry->lock, pTagEntry->valid);
	
	/* check Init status */
	RT_INIT_CHK(fb_cc_init);

	/* parameter check */
	RT_PARAM_CHK((pTagEntry == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((index >= RTL9603D_TABLESIZE_CAMTAG), RT_ERR_RG_INDEX_OUT_OF_RANGE);	
	
	if ((ret = rg_asic_table_write(RTL9603D_CAM_TAGt, index, (uint32 *)pTagEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	return RT_ERR_RG_OK;
}

rtk_rg_err_code_t rtk_rg_asic_camTagTable_get(uint32 index, rtk_rg_asic_camTag_entry_t *pTagEntry)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	
	/* check Init status */
	RT_INIT_CHK(fb_cc_init);
	
	/* parameter check */
	RT_PARAM_CHK((pTagEntry == NULL), RT_ERR_RG_NULL_POINTER);
	RT_PARAM_CHK((index >= RTL9603D_TABLESIZE_CAMTAG), RT_ERR_RG_INDEX_OUT_OF_RANGE);	
		
	if ((ret = rg_asic_table_read(RTL9603D_CAM_TAGt, index, (uint32 *)pTagEntry)) != RT_ERR_RG_OK)
	{
	    	RT_ERR(ret, (MOD_DAL|MOD_L34), "");
	    	return RT_ERR_FAILED;
	}
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "[CamTagTable] Get index [%d]: hashid %d lock %d valid %d", index, pTagEntry->hsahIdx, pTagEntry->lock, pTagEntry->valid);
	
	return RT_ERR_RG_OK;
}


rtk_rg_err_code_t rtk_rg_asic_switch_reset(void)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 tmpVal, retry;

	/* check Init status */
	// N/A

	/* parameter check */
	// N/A

	tmpVal = 1;
	if ((ret = reg_field_write(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
	{
		RT_ERR(ret, (MOD_L34|MOD_DAL), "");
	    	return ret;
	}

	retry = 0;
	do
	{
		if ((ret = reg_field_read(RTL9603D_SOFTWARE_RSTr, RTL9603D_SW_RSTf, (uint32 *)&tmpVal)) != RT_ERR_RG_OK)
		{
			return ret;
		}
		retry++;
	} while ((1 == tmpVal) && (retry < RETRY_TO_SHORT));

	
	if(retry >= RETRY_TO_SHORT)
		return RT_ERR_BUSYWAIT_TIMEOUT;
	
	return ret;
}

rtk_rg_err_code_t rtk_rg_asic_fb_init(void)
{
	rtk_rg_err_code_t ret = RT_ERR_RG_OK;
	uint32 chipId, chipRev	, chipSubtype;
	int i = 0;
	
	fb_init = INIT_COMPLETED;
	
	RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_L34), "Exec RG ASIC initilization!");

	
	rtk_switch_version_get(&chipId, &chipRev, &chipSubtype);
	
	if(chipId == RTL9603D_CHIP_ID){
		ASICDRIVERVER = chipRev;
	}

	ret = _rtk_rg_init_rgProDB();
	ret = rtk_rg_asic_table_reset(FB_RST_ALL);

	// reset flow mib counter
	{
		for(i = 0; i < RTL9603D_TABLESIZE_FLOWMIB; i++)
			rtk_rg_asic_flowMib_reset(i);
	}

	// reset ExtraTag
	{
		char emptyBuf[512];
		bzero(&emptyBuf, sizeof(emptyBuf));
		for(i = RTL9603D_EXTRATAG_LISTMIN; i <= RTL9603D_EXTRATAG_LISTMAX; i++)
			rtk_rg_asic_extraTagAction_del(i , 0);
		rtk_rg_asic_extraTagContentBuffer_set(0, &emptyBuf[0]);
	}
	
	/* Do other init process*/
	if(ret != RT_ERR_RG_OK)
		fb_init = INIT_NOT_COMPLETED;
	return RT_ERR_RG_OK;
}


