#if defined(__LUNA_KERNEL__)
#include <naf_kernel_util.h>
#include <linux/delay.h>
#else
#include <util.h>
#endif
#include <spi_nand/spi_nand_ctrl.h>
#include <spi_nand/spi_nand_common.h>
#include <spi_nand/spi_nand_util.h>
#include <ecc/ecc_struct.h>

/***********************************************
  *  Toshiba's ID Definition
  ***********************************************/
#define MID_TOSHIBA         (0x98)
#define DID_TC58CVG0S3HRAIG (0xC2)
#define DID_TC58CVG1S3HRAIG (0xCB)


// policy decision
    //input: #define NSU_PROHIBIT_QIO, or NSU_PROHIBIT_DIO  (in project/info.in)
    //          #define NSU_TOSHIBA_USING_QIO, NSU_TOSHIBA_USING_DIO, NSU_TOSHIBA_USING_SIO  (in project/info.in)
    //          #define NSU_DRIVER_IN_ROM, IS_RECYCLE_SECTION_EXIST (in template/info.in)
    //          #define NSU_USING_SYMBOL_TABLE_FUNCTION (in project/info.in)  

    //output: #define __DEVICE_REASSIGN, __DEVICE_USING_SIO, __DEVICE_USING_DIO, and __DEVICE_USING_QIO
    //            #define __SECTION_INIT_PHASE, __SECTION_INIT_PHASE_DATA
    //            #define __SECTION_RUNTIME, __SECTION_RUNTIME_DATA

#ifdef NSU_DRIVER_IN_ROM
    #define __SECTION_INIT_PHASE      SECTION_SPI_NAND
    #define __SECTION_INIT_PHASE_DATA SECTION_SPI_NAND_DATA
    #define __SECTION_RUNTIME         SECTION_SPI_NAND
    #define __SECTION_RUNTIME_DATA    SECTION_SPI_NAND_DATA
    #if defined(NSU_PROHIBIT_QIO) || defined(NSU_PROHIBIT_DIO)
        #error 'lplr should not run at ...'
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #error 'lplr should not have recycle section ...'
    #endif
    #define __DEVICE_USING_SIO 1
    #define __DEVICE_USING_DIO 0
    #define __DEVICE_USING_QIO 0
#else
    #ifdef NSU_USING_SYMBOL_TABLE_FUNCTION
        #define __DEVICE_REASSIGN 1
    #endif
    #ifdef IS_RECYCLE_SECTION_EXIST
        #define __SECTION_INIT_PHASE        SECTION_RECYCLE
        #define __SECTION_INIT_PHASE_DATA   SECTION_RECYCLE_DATA
        #define __SECTION_RUNTIME           SECTION_UNS_TEXT
        #define __SECTION_RUNTIME_DATA      SECTION_UNS_RO
    #else
        #define __SECTION_INIT_PHASE
        #define __SECTION_INIT_PHASE_DATA
        #define __SECTION_RUNTIME
        #define __SECTION_RUNTIME_DATA
    #endif

    #ifdef NSU_TOSHIBA_USING_QIO
        #if defined(NSU_PROHIBIT_QIO) && defined(NSU_PROHIBIT_DIO)
            #define __DEVICE_USING_SIO 1
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 0
        #elif defined(NSU_PROHIBIT_QIO) 
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 1
            #define __DEVICE_USING_QIO 0
        #else
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 1
        #endif
    #elif defined(NSU_TOSHIBA_USING_DIO)
        #if defined(NSU_PROHIBIT_DIO)
            #define __DEVICE_USING_SIO 1
            #define __DEVICE_USING_DIO 0
            #define __DEVICE_USING_QIO 0
        #else
            #define __DEVICE_USING_SIO 0
            #define __DEVICE_USING_DIO 1
            #define __DEVICE_USING_QIO 0
        #endif
    #else
        #define __DEVICE_USING_SIO 1
        #define __DEVICE_USING_DIO 0
        #define __DEVICE_USING_QIO 0
    #endif
#endif

//The Toshiba specific function
void toshiba_ecc_encode(u32_t ecc_ability, void *dma_addr, void *p_eccbuf);
s32_t toshiba_ecc_decode(u32_t ecc_ability, void *dma_addr, void *p_eccbuf);

#if __DEVICE_USING_QIO
__SECTION_INIT_PHASE_DATA
spi_nand_cmd_info_t toshiba_x4_cmd_info = {
    .w_cmd = PROGRAM_LOAD_OP,
    .w_addr_io = SIO_WIDTH,
    .w_data_io = SIO_WIDTH,
    .r_cmd = FAST_READ_X4_OP,
    .r_addr_io = SIO_WIDTH,
    .r_data_io = QIO_WIDTH,
    .r_dummy_cycles  = 8,
};
#endif

#ifdef NSU_DRIVER_IN_ROM
__SECTION_INIT_PHASE_DATA 
spi_nand_model_info_t toshiba_general_model = {
    ._pio_write = snaf_pio_write_data,
    ._pio_read = snaf_pio_read_data,
    ._page_read = snaf_page_read,
    ._page_write = snaf_page_write,
    ._page_read_ecc = snaf_page_read_with_ondie_ecc,
    ._page_write_ecc = snaf_page_write_with_ondie_ecc,
    ._block_erase = nsc_block_erase,
    ._wait_spi_nand_ready = nsc_wait_spi_nand_oip_ready,
};
#endif

__SECTION_INIT_PHASE_DATA
spi_nand_flash_info_t toshiba_chip_info[] = {
    {
        .man_id              = MID_TOSHIBA, 
        .dev_id              = DID_TC58CVG0S3HRAIG,
        ._num_block          = SNAF_MODEL_NUM_BLK_1024,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,        
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #elif __DEVICE_USING_SIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &nsc_sio_cmd_info,
            ._model_info     = &toshiba_general_model,
        #elif __DEVICE_USING_DIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &nsc_x2_cmd_info,
            ._model_info     = &toshiba_general_model,
        #elif __DEVICE_USING_QIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &toshiba_x4_cmd_info,
            ._model_info     = &toshiba_general_model,
        #endif
    },
    {
        .man_id              = MID_TOSHIBA, 
        .dev_id              = DID_TC58CVG1S3HRAIG,
        ._num_block          = SNAF_MODEL_NUM_BLK_2048,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_USE_ODE,        
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #elif __DEVICE_USING_SIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &nsc_sio_cmd_info,
            ._model_info     = &toshiba_general_model,
        #elif __DEVICE_USING_DIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &nsc_x2_cmd_info,
            ._model_info     = &toshiba_general_model,
        #elif __DEVICE_USING_QIO
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &toshiba_x4_cmd_info,
            ._model_info     = &toshiba_general_model,
        #endif
    },
    {//This is for Default
        .man_id              = MID_TOSHIBA, 
        .dev_id              = DEFAULT_DATA_BASE,
        ._num_block          = SNAF_MODEL_NUM_BLK_1024,
        ._num_page_per_block = SNAF_MODEL_NUM_PAGE_64,
        ._page_size          = SNAF_MODEL_PAGE_SIZE_2048B,
        ._spare_size         = SNAF_MODEL_SPARE_SIZE_64B,
        ._oob_size           = SNAF_MODEL_OOB_SIZE(24),
        ._ecc_ability        = ECC_MODEL_6T,
        #if __DEVICE_REASSIGN
            ._ecc_encode     = VZERO,
            ._ecc_decode     = VZERO,
            ._reset          = VZERO,
            ._cmd_info       = VZERO,
            ._model_info     = VZERO,
        #else
            ._ecc_encode     = toshiba_ecc_encode,
            ._ecc_decode     = toshiba_ecc_decode,
            ._reset          = nsu_reset_spi_nand_chip,
            ._cmd_info       = &nsc_sio_cmd_info,
            ._model_info     = &toshiba_general_model,
        #endif
    },
};

__SECTION_RUNTIME void 
toshiba_ecc_encode(u32_t ecc_ability, void *dma_addr, void *fake_ptr_cs)
{
    return;
}

__SECTION_RUNTIME s32_t 
toshiba_ecc_decode(u32_t ecc_ability, void *dma_addr, void *p_eccbuf)
{
    u8_t val = ((nsu_get_feature_reg(0xC0) >> 4)&0x3);
    return ((val==2)?ECC_USE_ODE_ERR:ECC_USE_ODE_SUCCESS);
}

__SECTION_INIT_PHASE u32_t 
toshiba_read_id(void)
{
    u32_t w_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));
    u32_t r_io_len = IO_WIDTH_LEN(SIO_WIDTH,CMR_LEN(2));
    u32_t ret = nsu_read_spi_nand_id(0, w_io_len, r_io_len);
    return ((ret>>16)&0xFFFF);
}

__SECTION_INIT_PHASE spi_nand_flash_info_t *
probe_toshiba_spi_nand_chip(void)
{
    nsu_reset_spi_nand_chip();

    #ifndef NSU_DRIVER_IN_ROM
    udelay(1100);
    #endif

    u32_t rdid = toshiba_read_id();
    if(MID_TOSHIBA != ((rdid >>8)&0xFF)) return VZERO;

    u16_t did8 = rdid &0xFF;
    u32_t i;   
    for(i=0 ; i<ELEMENT_OF_SNAF_INFO(toshiba_chip_info) ; i++){
        if((toshiba_chip_info[i].dev_id == did8) || (toshiba_chip_info[i].dev_id == DEFAULT_DATA_BASE)){
            #if __DEVICE_REASSIGN
                toshiba_chip_info[i]._cmd_info = _nsu_cmd_info_ptr;
                toshiba_chip_info[i]._model_info = &nsu_model_info;
                toshiba_chip_info[i]._reset = _nsu_reset_ptr;
                toshiba_chip_info[i]._ecc_encode= toshiba_ecc_encode;
                toshiba_chip_info[i]._ecc_decode= toshiba_ecc_decode;
                toshiba_chip_info[i]._model_info->_page_read_ecc = _nsu_page_read_with_ode_ptr;
                toshiba_chip_info[i]._model_info->_page_write_ecc = _nsu_page_write_with_ode_ptr;
            #endif //__DEVICE_REASSIGN

            nsu_enable_on_die_ecc();
            nsu_block_unprotect();
            return &toshiba_chip_info[i];
        }
    }
    return VZERO;
}

REG_SPI_NAND_PROBE_FUNC(probe_toshiba_spi_nand_chip);

