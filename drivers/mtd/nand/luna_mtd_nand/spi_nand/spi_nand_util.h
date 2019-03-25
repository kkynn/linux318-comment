#ifndef NAND_SPI_UTIL_H
#define NAND_SPI_UTIL_H
#include <soc.h>

#ifdef NSU_USING_SYMBOL_TABLE_FUNCTION
    #ifdef NSU_DRIVER_IN_ROM
        #error 'lplr should not use symbol_table function'
    #endif
    #include "spi_nand_symb_func.h"
#else
    #include "spi_nand_common.h"
    #define nsu_set_feature_reg nsc_set_feature_register
    #define nsu_reset_spi_nand_chip nsc_reset_spi_nand_chip
    #define nsu_disable_on_die_ecc nsc_disable_on_die_ecc
    #define nsu_enable_on_die_ecc  nsc_enable_on_die_ecc
    #define nsu_block_unprotect    nsc_block_unprotect
    #define nsu_read_spi_nand_id nsc_read_spi_nand_id
    #define nsu_get_feature_reg nsc_get_feature_register
    #define nsu_set_feature_reg nsc_set_feature_register
#endif

#define SNAF_BLOCK_ADDR(block_0_4095) (block_0_4095>>6)
#define SNAF_PAGE_ADDR(block_0_4095)  (block_0_4095&0x3F)


#if !defined(__LUNA_KERNEL__)
extern u32_t otto_plr_page_buf_addr;
#define page_buffer ((void*)(otto_plr_page_buf_addr))
#define oob_buffer   ((oob_t*)(OTTO_PLR_OOB_BUFFER))
#define ecc_buffer   ((void*)(OTTO_PLR_ECC_BUFFER))
#endif

typedef plr_oob_t oob_t;


int nsu_init(void);
int nsu_logical_page_read(void *data, u32_t page_num);
int nsu_probe(spi_nand_flash_info_t *info, const spi_nand_probe_t **, const spi_nand_probe_t **);

#endif


