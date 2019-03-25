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
#define MID_MICRON            (0x2C)
#define MID_DID_MT29F1G08ABA  (0x2CF18095) //1Gb=(2048 +   64) bytes / 64 pages / 1024 blocks, ECC 4-bits
#define MID_DID_MT29F2G08ABA  (0x2CDA9095) //2Gb=(2048 +   64) bytes / 64 pages / 2048 blocks, ECC 8-bits, ODE

/**** Chip Dependent Commmand ****/
#define FEATURE_ADDR_OPERATION_MODE (0x90)

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

__SECTION_INIT_PHASE void 
micron_enable_ode(void)
{
    ofu_set_feature(FEATURE_ADDR_OPERATION_MODE, 0x08000000);
}

__SECTION_INIT_PHASE onfi_info_t *
probe_micron_onfi_chip(onfi_info_t *info)
{
    u32_t readid = ofu_read_onfi_id();    

    if(MID_MICRON == ((readid >>24)&0xFF)){
        info->id_code = readid;

        if(MID_DID_MT29F2G08ABA == readid){
            micron_enable_ode();
            info->_num_block = ONFI_MODEL_NUM_BLK_2048;
        }else if(MID_DID_MT29F1G08ABA == readid){
            info->_num_block = ONFI_MODEL_NUM_BLK_1024;
        }else{
            info->_num_block = ONFI_MODEL_NUM_BLK_1024;
        }

        info->_num_page_per_block = ONFI_MODEL_NUM_PAGE_64;
        info->_page_size          = ONFI_MODEL_PAGE_SIZE_2048B;
        info->_spare_size         = ONFI_MODEL_SPARE_SIZE_64B;
        info->_oob_size           = ONFI_MODEL_OOB_SIZE(24);
        info->_ecc_ability        = ECC_MODEL_6T;       

        #ifdef ONFI_DRIVER_IN_ROM
            info->_ecc_encode= ecc_encode_bch;
            info->_ecc_decode= ecc_decode_bch;
            info->_reset     = ofc_reset_nand_chip;
            inline_memcpy(info->_model_info, &onfi_rom_general_model, sizeof(onfi_model_info_t));
        #else
            info->_ecc_encode= _ecc_encode_ptr;
            info->_ecc_decode= _ecc_decode_ptr;
            info->_reset     = _ofu_reset_ptr;
            info->_model_info= &onfi_plr_model_info;
        #endif

        return info;
    }
    return VZERO;
}

REG_ONFI_PROBE_FUNC(probe_micron_onfi_chip);


