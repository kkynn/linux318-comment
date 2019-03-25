/*
 * =====================================================================================
 *
 *       Filename:  onfi_common_chip.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *        Company:
 *
 * =====================================================================================
 */


#include <onfi_ctrl.h>
#include <onfi_common.h>
#include <onfi_util.h>
#include <onfi_struct.h>
#include <ecc_struct.h>
#include "naf_kernel_util.h"

#define ONFI_BYTES_PER_PAGE  2048
#define ONFI_PAGES_PER_BLK   64
#define ONFI_BLKS_PER_CHIP   2048


onfi_model_info_t onfi_plr_model_info SECTION_SDATA;
onfi_info_t onfi_plr_info SECTION_SDATA ={
    .id_code = 0,
    ._num_block = 0,
    ._num_page_per_block = 0,
    ._page_size = 0,
    ._spare_size = 0,
    ._oob_size = 0,
    ._ecc_ability = 0,
    ._ecc_encode = VZERO,
    ._ecc_decode = VZERO,
    ._reset = VZERO,
    ._model_info = &onfi_plr_model_info,
};

extern fpv_t  *_ofu_reset_ptr;
extern fpu32_t *_ofu_read_onfi_id;

//static onfi_info_t onfi_plr_info;

s32_t
onfi_probe(onfi_info_t *info, const onfi_probe_t **probe_func_start, const onfi_probe_t **probe_func_end)
{
    onfi_info_t *onaf_info=VZERO;

    const onfi_probe_t **probe_onfi = probe_func_start;
    while (probe_onfi != probe_func_end) {
        onaf_info = (*probe_onfi)(info);
        if(onaf_info != VZERO) {
            onfi_model_info_t *local_model_info = info->_model_info;
            memcpy(info,onaf_info,sizeof(onfi_info_t));
            memcpy(local_model_info, onaf_info->_model_info, sizeof(onfi_model_info_t));
            info->_model_info = local_model_info;
            return 0;
        }
        ++probe_onfi;
    }
    return -1;
}

extern const onfi_probe_t *LS_start_of_nand_onfi_probe_func;
extern const onfi_probe_t *LS_end_of_nand_onfi_probe_func;

int32_t _onfi_dummy_func(void) {return 0;}

extern ecc_encode_t *_ecc_encode_ptr;
extern ecc_decode_t *_ecc_decode_ptr;
extern ecc_engine_t *_ecc_engine_act_ptr;
extern fpv_t        *_opu_reset_ptr;

// Should globally exists
extern fps32_t            *_opu_chk_program_erase_sts_ptr;
extern fpu32_t            *_opu_read_onfi_id;
onfi_page_read_write_ecc_t  *_opu_page_read_with_ode_ptr  = (onfi_page_read_write_ecc_t *)_onfi_dummy_func;
onfi_page_read_write_ecc_t  *_opu_page_write_with_ode_ptr = (onfi_page_read_write_ecc_t *)_onfi_dummy_func;

void
init_onfi_plr_info(void)
{
	onfi_plr_info.id_code = _ofu_read_onfi_id();
	if(ONFI_BLKS_PER_CHIP == 1024){
		onfi_plr_info._num_block          = ONFI_MODEL_NUM_BLK_1024;
	}else if(ONFI_BLKS_PER_CHIP == 2048){
		onfi_plr_info._num_block          = ONFI_MODEL_NUM_BLK_2048;
	}else if(ONFI_BLKS_PER_CHIP == 4096){
		onfi_plr_info._num_block          = ONFI_MODEL_NUM_BLK_4096;
	}

	if(ONFI_PAGES_PER_BLK == 64){
		onfi_plr_info._num_page_per_block = ONFI_MODEL_NUM_PAGE_64;
	}else if(ONFI_PAGES_PER_BLK == 128){
		onfi_plr_info._num_page_per_block = ONFI_MODEL_NUM_PAGE_128;
	}

	if(2048 == ONFI_BYTES_PER_PAGE){
		onfi_plr_info._page_size   = ONFI_MODEL_PAGE_SIZE_2048B;
		onfi_plr_info._spare_size  = ONFI_MODEL_SPARE_SIZE_64B;
	}else if(4096 == ONFI_BYTES_PER_PAGE){
		onfi_plr_info._page_size   = ONFI_MODEL_PAGE_SIZE_4096B;
		onfi_plr_info._spare_size  = ONFI_MODEL_SPARE_SIZE_128B;
	}
	onfi_plr_info._oob_size    = ONFI_MODEL_OOB_SIZE(24);
	onfi_plr_info._ecc_ability = ECC_MODEL_6T;
	onfi_plr_info._ecc_encode  = _ecc_encode_ptr;
	onfi_plr_info._ecc_decode = _ecc_decode_ptr;
	onfi_plr_info._reset      = _ofu_reset_ptr;

	/*This is for translate onfi_gen2 to onfi_gen1*/
	u32_t temp = (ONFI_BYTES_PER_PAGE<<18)|(ONFI_BS_CMD_CYCLE(RFLD_NACFR(nafc_rc))<<14)|(ONFI_BS_ADDR_CYCLE(RFLD_NACFR(nafc_ac))<<10);
	onfi_plr_info.bs_cmd_cycle  = ((temp>>28)&0xF);
	onfi_plr_info.bs_addr_cycle = ((temp>>24)&0xF);
	onfi_plr_info.bs_mbz        = (temp&0xFFFFFF);
#if !defined(__LUNA_KERNEL__)
	if(onfi_util_init()<0){
	}
#endif
}

void show_onfi_info(struct onfi_info_s *info)
{
	printk("man_id is %x\n", info->man_id);
	printk("dev_id is %x\n", info->dev_id);
	printk("block number is %x(%x)\n", info->_num_block, ONFI_NUM_OF_BLOCK(info));
	printk("number of pages per block is %x(%x)\n", info->_num_page_per_block, ONFI_NUM_OF_PAGE_PER_BLK(info));
}
onfi_info_t *
getOnfiInfo(void)
{
	//init_onfi_plr_info();
	int result = onfi_probe(&onfi_plr_info, &LS_start_of_nand_onfi_probe_func, &LS_end_of_nand_onfi_probe_func );
	if (result) {
		return NULL;
	} else {
		onfi_plr_info.bs_cmd_cycle  = ONFI_BS_CMD_CYCLE(RFLD_NACFR(nafc_rc));
		onfi_plr_info.bs_addr_cycle = ONFI_BS_ADDR_CYCLE(RFLD_NACFR(nafc_ac));
		show_onfi_info(&onfi_plr_info);
		return &onfi_plr_info;
	}
}

u8_t (*onfi_get_status_register)(void) = ofc_status_read;

