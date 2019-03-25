#if defined(__LUNA_KERNEL__)
#include "naf_kernel_util.h"
#include <onfi_ctrl.h>
#include <onfi_common.h>
#include <onfi_util.h>
#else
#include <util.h>
#include <onfi/onfi_ctrl.h>
#include <onfi/onfi_common.h>
#include <onfi/onfi_util.h>
#endif

/****** ONFI ID Definition Table ******/
#define MID_MXIC             (0xC2)

/**** Chip Dependent Commmand ****/
#define FEATURE_ADDR_BLK_PROTECT 0xA0

#ifdef ONFI_DRIVER_IN_ROM
#if !defined(__LUNA_KERNEL__)
    #include <arch.h>
#endif
    #define __SECTION_INIT_PHASE      SECTION_ONFI
    #ifdef IS_RECYCLE_SECTION_EXIST
        #error 'lplr should not have recycle section ...'
    #endif
#else
    #ifdef ONFI_USING_SYMBOL_TABLE_FUNCTION
        #include <onfi/onfi_symb_func.h>   
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #define __SECTION_INIT_PHASE        SECTION_RECYCLE
    #else
        #define __SECTION_INIT_PHASE
    #endif
#endif


__SECTION_INIT_PHASE
void mxic_block_unprotect(void)
{
    ofu_set_feature(FEATURE_ADDR_BLK_PROTECT, 0);
}


__SECTION_INIT_PHASE onfi_info_t *
probe_mxic_onfi_chip(onfi_info_t *info)
{
    u32_t readid = ofu_read_onfi_id();

    if(MID_MXIC == ((readid >>24)&0xFF)){
        info->id_code = readid;

        u32_t did = (readid>>16)&0xFF;
        if(0xD3 == did){
            info->_num_block = ONFI_MODEL_NUM_BLK_8192;
        }else if(0xDC == did){
            info->_num_block = ONFI_MODEL_NUM_BLK_4096;
        }else if(0xDA == did){
            info->_num_block = ONFI_MODEL_NUM_BLK_2048;
        }else{ //0xF1 & Others
            info->_num_block = ONFI_MODEL_NUM_BLK_1024;
        }

        info->_num_page_per_block = ONFI_MODEL_NUM_PAGE_64;
        info->_page_size          = ONFI_MODEL_PAGE_SIZE_2048B;
        info->_spare_size         = ONFI_MODEL_SPARE_SIZE_64B;
        info->_oob_size           = ONFI_MODEL_OOB_SIZE(24);
        info->_ecc_ability        = ECC_MODEL_6T;

        #ifdef ONFI_DRIVER_IN_ROM
            info->_ecc_encode     = ecc_encode_bch;
            info->_ecc_decode     = ecc_decode_bch;
            info->_reset          = ofc_reset_nand_chip;
            inline_memcpy(info->_model_info, &onfi_rom_general_model, sizeof(onfi_model_info_t));
        #else
            info->_ecc_encode = _ecc_encode_ptr;
            info->_ecc_decode = _ecc_decode_ptr;
            info->_reset      = _ofu_reset_ptr;
            info->_model_info = &onfi_plr_model_info;
        #endif

        mxic_block_unprotect();

        return info;
    }
    return VZERO;
}

REG_ONFI_PROBE_FUNC(probe_mxic_onfi_chip);


