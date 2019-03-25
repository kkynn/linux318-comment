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
#include <ecc_struct.h>
#include "naf_kernel_util.h"

onfi_model_info_t common_onfi_model = {
	._pio_read       = onaf_pio_read_data,
	._pio_write      = onaf_pio_write_data,
	._page_read      = onaf_page_read,
	._page_write     = onaf_page_write,
	._page_read_ecc  = onaf_page_read_with_ecc_decode,
	._page_write_ecc = onaf_page_write_with_ecc_encode,
	._block_erase    = onfi_block_erase,
	._wait_onfi_rdy  = onfi_wait_nand_chip_ready,
};

static onfi_info_t onfi_plr_info = {
	._ecc_encode = ecc_encode_bch,
	._ecc_decode = ecc_decode_bch,
	._reset = onfi_reset_nand_chip,
	._model_info = &common_onfi_model,
};

//static onfi_info_t onfi_plr_info;

void
onfi_probe(onfi_info_t *info,
		const onfi_probe_t **probe_func_start,
		const onfi_probe_t **probe_func_end) {
	// result will be store in *info and its following structure
	onfi_info_t *onaf_info=VZERO;

	const onfi_probe_t **probe_onfi = probe_func_start;
	while (probe_onfi != probe_func_end) {
		onaf_info = (*probe_onfi)(info);
		if(onaf_info != VZERO) {
			return;
		}
		++probe_onfi;
	}
	return;
}

extern const onfi_probe_t *LS_start_of_nand_onfi_probe_func;
extern const onfi_probe_t *LS_end_of_nand_onfi_probe_func;

int32_t _onfi_dummy_func(void) {return 0;}

extern ecc_encode_t *_ecc_encode_ptr;
extern ecc_decode_t *_ecc_decode_ptr;
extern ecc_engine_t *_ecc_engine_act_ptr;
extern fpv_t        *_opu_reset_ptr;

// Should globally exists
extern onfi_get_sts_reg_t *_opu_get_sts_reg_ptr;
extern fps32_t            *_opu_chk_program_erase_sts_ptr;
extern fpu32_t            *_opu_read_onfi_id;
onfi_page_read_write_ecc_t  *_opu_page_read_with_ode_ptr  = (onfi_page_read_write_ecc_t *)_onfi_dummy_func;
onfi_page_read_write_ecc_t  *_opu_page_write_with_ode_ptr = (onfi_page_read_write_ecc_t *)_onfi_dummy_func;


void
init_onfi_plr_info(void)
{
	u32_t pin_nafc_rc = RFLD_NACFR(nafc_rc);
	u32_t pin_nafc_ac = RFLD_NACFR(nafc_ac);

	onfi_plr_info.bs_page_size  = ONFI_BS_PAGE_SIZE(pin_nafc_rc);
	onfi_plr_info.bs_cmd_cycle  = ONFI_BS_CMD_CYCLE(pin_nafc_rc);
	onfi_plr_info.bs_addr_cycle = ONFI_BS_ADDR_CYCLE(pin_nafc_ac);

	if(2048 == onfi_plr_info.bs_page_size){
		onfi_plr_info._page_size   = ONFI_MODEL_PAGE_SIZE_2048B;
		onfi_plr_info._spare_size  = ONFI_MODEL_SPARE_SIZE_64B;
		onfi_plr_info._ecc_ability = ECC_MODEL_6T;
	}else if(4096 == onfi_plr_info.bs_page_size){
		onfi_plr_info._page_size   = ONFI_MODEL_PAGE_SIZE_4096B;
		onfi_plr_info._spare_size  = ONFI_MODEL_SPARE_SIZE_128B;
		onfi_plr_info._ecc_ability = ECC_MODEL_6T;
	}

	onfi_plr_info._num_block          = ONFI_MODEL_NUM_BLK_1024;
	onfi_plr_info._num_page_per_block = ONFI_MODEL_NUM_PAGE_64;
	onfi_plr_info._oob_size           = ONFI_MODEL_OOB_SIZE(24);

	onfi_plr_info._reset      = _opu_reset_ptr;
	onfi_plr_info._ecc_encode = _ecc_encode_ptr;
	onfi_plr_info._ecc_decode = _ecc_decode_ptr;

#if !defined(__LUNA_KERNEL__)
	//Probe for Toshiba specific chip
	init_onfi_probe();
#else
	onfi_probe(&onfi_plr_info, &LS_start_of_nand_onfi_probe_func, &LS_end_of_nand_onfi_probe_func );
#endif

	//Read ID:
	if(onfi_plr_info.man_id == 0){
		u32_t rdid  = _opu_read_onfi_id();
		onfi_plr_info.man_id = (rdid >> 16)& 0xFFFF;
		onfi_plr_info.dev_id = rdid & 0xFFFF;
	}

#if !defined(__LUNA_KERNEL__)
	// basic parallel-nand init and establish the plr_mapping_table
	if(onfi_util_init()<0){
		puts(_onfi_u_table_init_fail_msg);
		while(1);
	}
#endif

}

onfi_info_t *
getOnfiInfo(void)
{
	init_onfi_plr_info();
	return &onfi_plr_info;
}


