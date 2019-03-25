/******************************************************************************
 * $Id: luan_spi_nand.c,v 1.0 2016/06/13   Exp $
 * drivers/mtd/nand/rtk_nand.c
 * Overview: Realtek NAND Flash Controller Driver
 * Copyright (c) 2016 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2016-06-13 Yang, ChaoYuan   create file
 *
 *******************************************************************************/


#define DEBUG

#include <linux/device.h>
#undef DEBUG
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <mtd/mtd-abi.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "spi_nand_ctrl.h"
#include "spi_nand_common.h"
#include "spi_nand/spi_nand_struct.h"
#include "ecc/ecc_struct.h"
#include "snaf_kernel_util.h"
#include <linux/module.h>
#include "../../mtdcore.h"
#include <linux/dma-mapping.h>

#define BANNER  "Realtek Luna SPI NAND Flash Driver"
#define VERSION  "$Id: luna_spi_nand.c,2016/06/13 00:00:00 ChaoYuan_Yang Exp $"

#define MAX_ALLOWED_ERR_IN_BLANK_PAGE (4)
#define USE_SECOND_BBT 1
#define USE_BBT_SKIP 1
#define RTK_MAX_FLASH_NUMBER 2

static uint32_t verify_page_addr;
/* inline function */
inline static void
_set_flags(u32_t *arr, u32_t i) {
	unsigned idx=i/(8*sizeof(u32_t));
	i &= (8*sizeof(u32_t))-1;
	arr[idx] |= 1UL << i;
}

inline static int
_get_flags(u32_t *arr, u32_t i) {
	unsigned idx=i/(8*sizeof(u32_t));
	i &= (8*sizeof(u32_t))-1;
	return (arr[idx] & (1UL << i)) != 0;
}


struct luna_nand_t {
	struct nand_chip chip;
	struct spi_nand_flash_info_s *_spi_nand_info;
	u8_t *_oob_poi;
	unsigned int _writesize;
	unsigned int _spare_size;
	unsigned int readIndex;
	unsigned int writeIndex;
	unsigned int pre_command;
	unsigned int address_shift; // block number shift for different nand chip
	unsigned int static_shift;  // static block number chip for use different nand chip
	int selected_chip;
	int column;
	int page_addr;
	union {
		unsigned int v;
		unsigned char b[4];
		unsigned short w[2];
	}status;
	u8_t *_bbt;
	u32_t *_bbt_table;
	unsigned char *_ecc_buf;
	unsigned char *_page_buf;
#if USE_SECOND_BBT
	u32_t *_bbt_table2;
#endif
};

static struct mtd_info *rtk_mtd;
static struct spi_nand_flash_info_s spi_nand_flash_info;
static struct spi_nand_flash_info_s *_spi_nand_info = &spi_nand_flash_info;

static u32_t RTK_NAND_PAGE_BUF_SIZE = (MAX_PAGE_BUF_SIZE+MAX_OOB_BUF_SIZE);
/* NOTE: Need to be changed if use a flash with more block */
#define MAX_BLOCKS (1024<<3)

static unsigned char *_ecc_buf  = NULL;
static unsigned char *_page_buf = NULL;
static u8_t *_bbt               = NULL;
static u32_t *_bbt_table        = NULL;
#if USE_SECOND_BBT
static u32_t *_bbt_table2       = NULL;
#endif


static struct nand_ecclayout nand_bch_oob_64 = {
	.eccbytes = 44, //ecc 40byte, + 4 bbi
	.eccpos = {
			24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
			34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
			44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 20}}
};

static struct nand_ecclayout nand_bch_oob_128 = {
	.eccbytes = 44+40, //ecc 40byte, + 4 bbi + ecc 40byte
	.eccpos = {
			24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
			34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
			44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
			54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
			88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
			98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
			108, 109, 110/*, 111, 112, 113, 114, 115, 116, 117,
			118, 119, 120, 121, 122, 123, 124, 122, 126, 127 */
			/* because that eccpos is declared in length 32 only */
	},
	.oobfree = {
		{.offset = 2,
		 .length = 20},
		{.offset = 64,
		.length = 24}}
};



#ifdef CONFIG_MTD_CMDLINE_PARTS
/* for fixed partition */
const char *ptypes[] = {"cmdlinepart", NULL};
//const char *ptypes[] = {"mtdparts=rtk_nand:640k@0(boot),6M@0x180000(linux),-(rootfs)", NULL};
#else
//eric, use cmdlinepart now
static struct mtd_partition rtl8686_parts[] = {
	{ name: "boot",		offset: 0,		size:  0xc0000, mask_flags: 0},
	{ name: "env",		offset: 0xc0000,	size: 0xe0000, mask_flags:0},
	{ name: "env2",		offset: 0xe0000,	size: 0x100000, mask_flags:0},
	{ name: "config",	offset: 0x100000,	size: 0x1900000, mask_flags:0},
	{ name: "k0",		offset: 0x1900000,	size: 0x2100000, mask_flags:0},
	{ name: "r0",		offset: 0x2100000,	size: 0x4b00000, mask_flags:0},
	{ name: "k1",		offset: 0x4b00000,	size: 0x5300000, mask_flags:0},
	{ name: "r1",		offset: 0x5300000,	size: 0x7d00000, mask_flags:0},
	{ name: "opt4",		offset: 0x7d00000,	size: 0x8000000, mask_flags:0},
	{ name: "Partition_009",offset: 0,		size: 0x1000, mask_flags:0},
	{ name: "Partition_010",offset: 0,		size: 0x1000, mask_flags:0},
	{ name: "linux",	offset: 0x1900000,	size: 0x2100000, mask_flags:0},
	{ name: "rootfs",	offset: 0x2100000,	size: 0x4b00000, mask_flags:0},
};
#endif


static int
rtk_pio_read (struct mtd_info *mtd, u16_t chipnr, int page_id, int column, int len, u_char *buf)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	chip->_spi_nand_info->_model_info->_pio_read(chip->_spi_nand_info, buf, len, page_id, column);
	return 0;
}

static int
is_plr_first_load_part(struct mtd_info *mtd, u32_t blk_pge_addr)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	plr_first_load_layout_t *fl_buf=(plr_first_load_layout_t *)chip->_page_buf;

	chip->_spi_nand_info->_model_info->_page_read(chip->_spi_nand_info, chip->_page_buf, blk_pge_addr);

	if(ecc_engine_action(((u32_t)chip->_spi_nand_info->_ecc_ability), fl_buf->data0, fl_buf->oob0, 0) == ECC_CTRL_ERR) return ECC_CTRL_ERR;
	if(ecc_engine_action(((u32_t)chip->_spi_nand_info->_ecc_ability), fl_buf->data1, fl_buf->oob1, 0) == ECC_CTRL_ERR) return ECC_CTRL_ERR;
	if(PLR_FL_GET_SIGNATURE(fl_buf) == SIGNATURE_PLR_FL) return 1;

	return 0;
}


static int
create_bbt(struct mtd_info *mtd, u8_t *buf, struct nand_bbt_descr *bd, int chip_no)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	int blk_shift = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift = (((struct nand_chip*)chip)->page_shift) ;
	u8_t bb_tag[4];
	u32_t *bbt_table = (u32_t*)buf;
	uint32_t bn;
	int32_t ret;

	for (bn = 1; bn < SNAF_NUM_OF_BLOCK(chip->_spi_nand_info); bn++) {
#if USE_SECOND_BBT
		rtk_pio_read(mtd, 0, (bn<<blk_shift)>>page_shift, chip->_writesize+20, 4, bb_tag);
		if (0 == bb_tag[2] && 1 != is_plr_first_load_part(mtd, bn)) {
			printk("BBT2 scan %d as bad\n", bn);
			_set_flags(chip->_bbt_table2, bn);
		}
#endif
		rtk_pio_read(mtd, 0, (bn<<blk_shift)>>page_shift, chip->_writesize, 4, bb_tag);
		if (0xFF == bb_tag[0]) {
			continue;
		}
		ret = chip->_spi_nand_info->_model_info->_page_read_ecc(chip->_spi_nand_info, chip->_page_buf, (bn<<blk_shift)>>page_shift, chip->_ecc_buf);
		if ( ! IS_ECC_DECODE_FAIL(ret)
				&& 0xFF == chip->_page_buf[chip->_writesize]) {
			continue;
		}

		_set_flags(bbt_table, bn);
	}
	return 0;
}

static int
create_logical_skip_bbt(struct mtd_info *mtd, u32_t *bbt_table)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	unsigned int good_num;
	int j;
	unsigned int bbi_block;
	unsigned int skip_block;

	skip_block = good_num = bbi_block = 1;
	for (;bbi_block < SNAF_NUM_OF_BLOCK(chip->_spi_nand_info);bbi_block++) {
		j=_get_flags(bbt_table, bbi_block);

		if (j==0) { //good block
			chip->_bbt[skip_block]=bbi_block-good_num;
			good_num++;
			skip_block++;
		} else {
			printk("detect block %d is bad \n",bbi_block);
		}
	}
	printk("good block number %d\n", skip_block);
	for (;skip_block< SNAF_NUM_OF_BLOCK(chip->_spi_nand_info); skip_block++) {
		chip->_bbt[skip_block]=0xff;
	}

	return 0;

    //for (skip_block=0;skip_block<20;skip_block++) {
    //	printf("this->bbt[%d]= %d\n",skip_block,chip->_bbt[skip_block]);
    //}
}

static inline int
get_good_block(struct mtd_info *mtd, int *page_id)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int blk_shift = (this->phys_erase_shift - this->page_shift);
	int blk_num = (*page_id)>>blk_shift;

	u32_t skip_num = chip->_bbt[blk_num];
	if ( skip_num == 0xFF) {
		return -1;
	}
	*page_id = ((blk_num + skip_num)<<blk_shift) + (((1<<blk_shift)-1) & (*page_id));

	return 0;
}


static u8_t
rtk_nand_read_byte(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	if (chip->readIndex > 3) {
		return 0;
	}

	return chip->status.b[chip->readIndex++];

}

static u16_t
rtk_nand_read_word(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	u16_t reval;
	if (chip->readIndex > 3) {
		return 0;
	}

	chip->readIndex &= -2;
	reval = *((u16_t*)(&chip->status.w[(chip->readIndex)>>1]));
	chip->readIndex += 2;
	return reval;
}

static void
rtk_nand_write_buf(struct mtd_info *mtd, const u8_t *buf, int len)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	if (chip->column+len > RTK_NAND_PAGE_BUF_SIZE) {
		return;
	}
	memcpy( chip->_page_buf+chip->column, buf, len);
	chip->column += len;
}

static void
rtk_nand_read_buf(struct mtd_info *mtd, u8_t *buf, int len)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	len = len < (RTK_NAND_PAGE_BUF_SIZE -  chip->column ) ? len : (RTK_NAND_PAGE_BUF_SIZE -  chip->column );
	memcpy( buf, chip->_page_buf+chip->column, len);
	chip->readIndex = chip->column + len;
	chip->column +=   len;
}

static void
rtk_nand_select_chip(struct mtd_info *mtd, int chip_num)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	chip->selected_chip = chip_num;
	if (chip_num >= RTK_MAX_FLASH_NUMBER) {
		printk("Select not correct Flash Number %d\n", chip_num);
		BUG();
	}
}


#if  USE_SECOND_BBT
/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @mtd: MTD device structure
 * @new_state: the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int
nand_get_device(struct mtd_info *mtd, int new_state)
{
	struct nand_chip *chip = mtd->priv;
	spinlock_t *lock = &chip->controller->lock;
	wait_queue_head_t *wq = &chip->controller->wq;
	DECLARE_WAITQUEUE(wait, current);
retry:
	spin_lock(lock);

	/* Hardware controller shared among independent devices */
	if (!chip->controller->active)
		chip->controller->active = chip;

	if (chip->controller->active == chip && chip->state == FL_READY) {
		chip->state = new_state;
		spin_unlock(lock);
		return 0;
	}
	if (new_state == FL_PM_SUSPENDED) {
		if (chip->controller->active->state == FL_PM_SUSPENDED) {
			chip->state = FL_PM_SUSPENDED;
			spin_unlock(lock);
			return 0;
		}
	}
	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(wq, &wait);
	spin_unlock(lock);
	schedule();
	remove_wait_queue(wq, &wait);
	goto retry;
}

/**
 * nand_release_device - [GENERIC] release chip
 * @mtd: MTD device structure
 *
 * Release chip lock and wake up anyone waiting on the device.
 */
static void nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	/* Release the controller and the chip */
	spin_lock(&chip->controller->lock);
	chip->controller->active = NULL;
	chip->state = FL_READY;
	wake_up(&chip->controller->wq);
	spin_unlock(&chip->controller->lock);
}




static int
rtk_query_bbt2(struct mtd_info *mtd, int blk_num)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	if (_get_flags(chip->_bbt_table2, blk_num)) {
		printk("BBT2 shows that %d is bad\n", blk_num);
	}
	return _get_flags(chip->_bbt_table2, blk_num);

}

static int
rtk_mark_bbt2(struct mtd_info *mtd, int blk_num)
{
	u8_t status = 0;
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	int blk_shift = (((struct nand_chip*)chip)->phys_erase_shift);
	int page_shift = (((struct nand_chip*)chip)->page_shift) ;
	int page_addr = (blk_num<<blk_shift)>>page_shift;
	memset(chip->_page_buf, 0x00, mtd->writesize+mtd->oobsize);
	chip->_page_buf[mtd->writesize]   = 0xFF;
	chip->_page_buf[mtd->writesize+1] = 0xFF;
	status = chip->_spi_nand_info->_model_info->_block_erase(chip->_spi_nand_info, page_addr);
	if (!status)
		status = chip->_spi_nand_info->_model_info->_page_write_ecc(chip->_spi_nand_info, chip->_page_buf, page_addr, chip->_ecc_buf);
	if (!status)
		_set_flags(chip->_bbt_table2, blk_num);

	return status;
}

static int
rtk_mark_bbt2_r(struct mtd_info *mtd, int blk_num)
{
	u8_t status = 0;
	nand_get_device(mtd, FL_WRITING);
	status = rtk_mark_bbt2(mtd, blk_num);
	nand_release_device(mtd);
	return status;
}
#endif

static int
rtk_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{

	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	unsigned int phy_block, logical_block;

	logical_block = ofs>>this->phys_erase_shift;

	/*return bad block table */

#ifdef USE_BBT_SKIP
	//if use logical block ,always return good

	phy_block = logical_block + chip->_bbt[logical_block];
	if (chip->_bbt[logical_block] >= 0xff) {
		return 1;
	} else {
#if USE_SECOND_BBT
		return rtk_query_bbt2(mtd, phy_block);
#endif
		return 0;
	}
#else

	/* read bad table array, return 1 is bad , return 0 is good */
	int j = _get_flags (chip->_bbt_table, logical_block );
	return j;
#endif
}

static int
rtk_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	unsigned int chunk_id,block;
	unsigned int phy_block, logical_block;

#ifndef USE_BBT_SKIP
	u8 nand_buffer[2048+64];
#endif
	chunk_id = ((int) ofs) >> this->page_shift;

	block = chunk_id>>(this->phys_erase_shift);

#ifdef USE_BBT_SKIP
#if USE_SECOND_BBT
	logical_block = ofs>>this->phys_erase_shift;
	phy_block = logical_block + chip->_bbt[logical_block];
	if (rtk_query_bbt2(mtd, phy_block)) {
		return 0;
	}
	return rtk_mark_bbt2(mtd, phy_block);
#endif
	//if use logical block bbt, always return fail
	return 1;
#else

	/* set bad table array*/
	printk("Application set block %d is bad \n");
	_set_flags (chip->_bbt_table, block);
	memset (nand_buffer, 0x0, sizeof(nand_buffer));
	//pio write to chunk
	//TODO write BBI mark to flash
	//rtk_PIO_write(chunk_id,0,chunk_size+oob_size,&nand_buffer[0]);

	return 0;
#endif

}

static int
rtk_nand_waitfunc(struct mtd_info* mtd, struct nand_chip *this)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	// it will returh the status
	return chip->status.b[0];
}


static void
rtk_nand_cmdfunc(struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	switch (command) {
	case NAND_CMD_READ0:
		get_good_block(mtd, &page_addr);
		chip->_spi_nand_info->_model_info->_page_read(chip->_spi_nand_info, chip->_page_buf, page_addr);
		verify_page_addr = page_addr;
		chip->column = column;
		chip->page_addr = page_addr;


		break;
	case NAND_CMD_READOOB:
		get_good_block(mtd, &page_addr);
		chip->_spi_nand_info->_model_info->_page_read(chip->_spi_nand_info, chip->_page_buf, page_addr);
		column = chip->_writesize;
		chip->column = column;
		chip->page_addr = page_addr;
		break;
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
		chip->status.v = 0;
		chip->status.b[0] = chip->_spi_nand_info->_model_info->_page_write(chip->_spi_nand_info, chip->_page_buf, chip->page_addr);
		chip->readIndex = 0;
		break;
	case NAND_CMD_SEQIN:
		get_good_block(mtd, &page_addr);
		chip->pre_command = NAND_CMD_SEQIN;
		chip->column = column;
		chip->page_addr = page_addr;
		break;
	case NAND_CMD_STATUS:
        chip->status.v = 0;
		chip->status.b[0] = nsc_get_feature_register(0xC0) | 0x80;
		chip->readIndex = 0;
		break;
	case NAND_CMD_RESET:
		chip->_spi_nand_info->_reset();
		break;
	case NAND_CMD_READID:
		chip->status.v = 0;
		chip->status.b[0] = chip->_spi_nand_info->man_id;
		chip->status.b[1] = (chip->_spi_nand_info->dev_id)>>8;
		if(0 == chip->status.b[1])
			chip->status.b[1] = (chip->_spi_nand_info->dev_id);
		chip->status.b[2] = (chip->_spi_nand_info->dev_id);
		chip->readIndex = 0;
		break;
	case NAND_CMD_ERASE1:
		chip->readIndex = 0;
		chip->status.v = 0;
		get_good_block(mtd, &page_addr);
		chip->status.b[0] = chip->_spi_nand_info->_model_info->_block_erase(chip->_spi_nand_info, page_addr);
	case NAND_CMD_ERASE2:
		break;

		/*
		 * read error status commands require only a short delay
		 */

	case NAND_CMD_RNDOUT:
	case NAND_CMD_RNDIN:


	default:
		printk("Use NOT SUPPORTED command in NAND flash!\n");
		BUG();
		break;
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */

	}
	chip->pre_command = command;
	return;
}


static int
rtk_nand_scan_bbt(struct mtd_info *mtd)
{
	register struct luna_nand_t *chip = mtd->priv;

	if (SNAF_NUM_OF_BLOCK(chip->_spi_nand_info) > MAX_BLOCKS) {
		printk("Create BBT Error!, Block number > %d\n", MAX_BLOCKS);
	} else {
		create_bbt(mtd, (u8_t*)chip->_bbt_table, NULL, 0);
		//create bbt
#ifdef USE_BBT_SKIP
		create_logical_skip_bbt(mtd, chip->_bbt_table);
#endif
	}

	return 0;
}

static void
rtk_nand_bug(struct mtd_info *mtd)
{
	printk("Use an unsupported function in MTD NAND driver");
	BUG();
}

static int
rtk_nand_check_blank(struct mtd_info *mtd)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	uint32_t *end_pos    = (uint32_t *)(chip->_page_buf+(chip->_writesize + chip->_spare_size));
	uint32_t *start_pos  = (uint32_t *)chip->_page_buf;
	uint32_t error_bit_n = 0;
	uint32_t val;
	do {
		val = 0xFFFFFFFF ^ (*start_pos);
		while (val) {
			error_bit_n += 1;
			if (error_bit_n > MAX_ALLOWED_ERR_IN_BLANK_PAGE)
				return -1;
			val &= val - 1;
		}
	} while (++start_pos != end_pos);
	return 0;
}

static int
rtk_nand_read_page(struct mtd_info *mtd,
					     struct nand_chip *_chip,
					     u8_t *buf, int oob_required, int page)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	int ret;

	ret = chip->_spi_nand_info->_ecc_decode(ECC_CORRECT_BITS(chip->_spi_nand_info) , chip->_page_buf, chip->_ecc_buf);
	if (IS_ECC_DECODE_FAIL(ret) == 1) {
		printk("[SPINAND:] ECC ERROR(ret=%08X) page_addr = %04X! Do retry!\n", ret, verify_page_addr);
		chip->_spi_nand_info->_model_info->_page_read(chip->_spi_nand_info, chip->_page_buf, verify_page_addr);
		ret = chip->_spi_nand_info->_ecc_decode(ECC_CORRECT_BITS(chip->_spi_nand_info) , chip->_page_buf, chip->_ecc_buf);
	}
	memcpy( buf, chip->_page_buf, chip->_writesize) ;
	memcpy(_chip->oob_poi, chip->_oob_poi, chip->_spare_size);
	if (IS_ECC_DECODE_FAIL(ret) == 1) {
		printk("[SPINAND:] ECC ERROR(ret=%08X) page_addr = %04X!\n", ret, verify_page_addr);
		return rtk_nand_check_blank(mtd);
	} else {
		return 0;
	}
}

static int
rtk_nand_write_page(struct mtd_info *mtd,
					      struct nand_chip *_chip,
					      const uint8_t *buf, int oob_required)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	memcpy(chip->_page_buf, buf, chip->_writesize) ;
	memcpy(chip->_oob_poi, _chip->oob_poi, chip->_spare_size);
	chip->_spi_nand_info->_ecc_encode(ECC_CORRECT_BITS(chip->_spi_nand_info) , chip->_page_buf, _ecc_buf);
	return 0;
}

/*************************************************************************
**  spi_nand_detect()
**	descriptions:
**	parameters:
**	return:
**  note:
*************************************************************************/
extern const spi_nand_probe_t *LS_start_of_nand_spif_probe_func;
extern const spi_nand_probe_t *LS_end_of_nand_spif_probe_func;

static int
rtk_spi_nand_detect (void)
{
	const spi_nand_probe_t **pf = &LS_start_of_nand_spif_probe_func;
	void *flash_info = VZERO;


	while (pf != &LS_end_of_nand_spif_probe_func) {
		flash_info = (void *)(*pf)();
		if (flash_info != VZERO) {
			memcpy(&spi_nand_flash_info, flash_info, sizeof(struct spi_nand_flash_info_s));
			return 1;
		}
		pf++;
	}

	return 0;
}

#define LUNA_SPI_OPTION (NAND_NO_SUBPAGE_WRITE)
static void
rtk_nand_ids(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	nand_flash_ids[0].name = "\0";
	nand_flash_ids[0].mfr_id = chip->_spi_nand_info->man_id;
	if (chip->_spi_nand_info->dev_id & 0xFF00) {
		nand_flash_ids[0].dev_id = ((chip->_spi_nand_info->dev_id)>>8)&0xFF;
	} else {
		nand_flash_ids[0].dev_id = (chip->_spi_nand_info->dev_id)&0xFF;
	}
	nand_flash_ids[0].pagesize = SNAF_PAGE_SIZE(chip->_spi_nand_info);
	nand_flash_ids[0].chipsize = (SNAF_NUM_OF_BLOCK(chip->_spi_nand_info)*SNAF_NUM_OF_PAGE_PER_BLK(chip->_spi_nand_info)*SNAF_PAGE_SIZE(chip->_spi_nand_info))>>20;
	nand_flash_ids[0].erasesize = SNAF_NUM_OF_PAGE_PER_BLK(chip->_spi_nand_info)*SNAF_PAGE_SIZE(chip->_spi_nand_info);
	nand_flash_ids[0].options = LUNA_SPI_OPTION;
	nand_flash_ids[0].oobsize = SNAF_SPARE_SIZE(chip->_spi_nand_info);
	nand_flash_ids[0].ecc.strength_ds = 4;
	nand_flash_ids[0].ecc.step_ds = 512;
	// for the case that read back 8 byte full id info
	nand_flash_ids[0].id_len = 0;

}//static void rtk_nand_ids()


static void
rtk_nand_set(struct mtd_info *mtd)
{
	register struct nand_chip *chip = (struct nand_chip *)mtd->priv;

	if (!chip->chip_delay)
		chip->chip_delay = 20;

	chip->cmdfunc = rtk_nand_cmdfunc;

	chip->waitfunc = rtk_nand_waitfunc;
	//chip->erase_cmd = rtk_nand_erase_cmd;

	chip->read_byte = rtk_nand_read_byte;
	chip->read_word = rtk_nand_read_word;
	chip->select_chip = rtk_nand_select_chip;
#ifdef USE_BBT_SKIP
	chip->block_bad = rtk_nand_block_bad;
	chip->block_markbad = rtk_nand_block_markbad;
	printk("Use RTK bb func\n");
#else
	printk("Use nandbase bb func\n");
#endif
	chip->write_buf = rtk_nand_write_buf;
	chip->read_buf = rtk_nand_read_buf;
#ifdef USE_BBT_SKIP
	chip->scan_bbt = rtk_nand_scan_bbt;
	printk("Use RTK bb scan\n");
#else
	printk("Use nandbase bb scan\n");
#endif

}

static void
rtk_luna_nand_set(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	chip->_writesize = SNAF_PAGE_SIZE(chip->_spi_nand_info);
	chip->_oob_poi =  (chip->_page_buf) + (chip->_writesize);
	chip->_spare_size = SNAF_SPARE_SIZE(chip->_spi_nand_info);
}

static void
rtk_luna_nand_chk(struct mtd_info *mtd)
{
	register struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;

	if (mtd->writesize != SNAF_PAGE_SIZE(chip->_spi_nand_info)) {
		printk("SPI NAND: ERROR! Page size info not match, please check id table!\n");
		BUG();
	}

}

static int
rtk_nand_ecc_set(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	register struct nand_ecc_ctrl *ecc = &(((struct nand_chip *)mtd->priv)->ecc);

	ecc->mode = NAND_ECC_HW;
	ecc->size = 512;
	ecc->bytes = 10;
	if (2048 == chip->_writesize) {
		ecc->layout = &nand_bch_oob_64;
	} else if (4096 == chip->_writesize ) {
		ecc->layout = &nand_bch_oob_128;
	} else {
		printk("%s: Error! unsupported page size %d\n", __FUNCTION__, chip->_writesize);
		return -1;
	}

	ecc->calculate = (void*)rtk_nand_bug;
	ecc->correct = (void*)rtk_nand_bug;

	ecc->read_page = rtk_nand_read_page;
	ecc->write_page = rtk_nand_write_page;
	ecc->read_subpage = (void*)rtk_nand_bug;
	ecc->strength = 6;
	return 0;
}


static int rtk_mtd_create_buffer(struct mtd_info *mtd);
/*************************************************************************
**  rtk_spi_nand_profile()
**	descriptions: rtk luna spi nand driver init function
**	parameters:
**	return:
**  note:
*************************************************************************/
static int
rtk_spi_nand_profile (void)
{
	char *ptype;
	int pnum = 0;
	struct mtd_partition *mtd_parts;
	register struct luna_nand_t *chip = (struct luna_nand_t *)rtk_mtd->priv;


	if ((rtk_spi_nand_detect ()) == 0) {
		printk ("Warning: Lookup Table do not have this nand flash\n");
		return -1;
	}

	chip->_spi_nand_info = _spi_nand_info;
	if (rtk_mtd_create_buffer(rtk_mtd)) {
		return -1;
	}

	rtk_luna_nand_set(rtk_mtd);

	rtk_nand_set(rtk_mtd);
	if (rtk_nand_ecc_set(rtk_mtd))
		return -1;

	rtk_nand_ids(rtk_mtd);
	if(nand_scan(rtk_mtd, 1)){
		printk ("Warning: rtk nand scan error!\n");
		return -1;
	}

	rtk_luna_nand_chk(rtk_mtd);

	/* check page size(write size) is 512/2048/4096.. must 512byte align */
	if (!(rtk_mtd->writesize & (0x200 - 1)))
		;//rtk_writel( rtk_mtd->oobblock >> 9, REG_PAGE_LEN);
	else {
		printk ("Error: pagesize is not 512Byte Multiple");
		return -1;
	}

#ifdef CONFIG_MTD_CMDLINE_PARTS
	/* partitions from cmdline */
	ptype = (char *)ptypes[0];
	pnum = parse_mtd_partitions (rtk_mtd, ptypes, &mtd_parts, 0);

	if (pnum <= 0) {
		printk (KERN_NOTICE "RTK: using the whole nand as a partitoin\n");
		if (add_mtd_device (rtk_mtd)) {
			printk (KERN_WARNING "RTK: Failed to register new nand device\n");
			return -EAGAIN;
		}
	} else {
		printk (KERN_NOTICE "RTK: using dynamic nand partition\n");
		if (add_mtd_partitions (rtk_mtd, mtd_parts, pnum)) {
			printk("%s: Error, cannot add %s device\n",
					__FUNCTION__, rtk_mtd->name);
			rtk_mtd->size = 0;
			return -EAGAIN;
		}
	}
#else
	/* fixed partition ,modify rtl8686_parts table*/
	if (add_mtd_device (rtk_mtd)) {
		printk(KERN_WARNING "RTK: Failed to register new nand device\n");
		return -EAGAIN;
	}

	if (add_mtd_partitions (rtk_mtd, rtl8686_parts, ARRAY_SIZE(rtl8686_parts))) {
			printk("%s: Error, cannot add %s device\n",
					__FUNCTION__, rtk_mtd->name);
			rtk_mtd->size = 0;
			return -EAGAIN;
	}

	ROOT_DEV = MKDEV(MTD_BLOCK_MAJOR, 0);
#endif
	return 0;
}

static dma_addr_t _ecc_buf_addr;
static dma_addr_t _page_buf_addr;

static int
rtk_mtd_create_buffer(struct mtd_info *mtd)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)mtd->priv;
	const int bbt_table_len = (sizeof(*_bbt_table))*(NUM_WORD(SNAF_NUM_OF_BLOCK(chip->_spi_nand_info)));

	if (RTK_NAND_PAGE_BUF_SIZE < (SNAF_PAGE_SIZE(chip->_spi_nand_info) + SNAF_SPARE_SIZE(chip->_spi_nand_info))) {
		RTK_NAND_PAGE_BUF_SIZE = (SNAF_PAGE_SIZE(chip->_spi_nand_info) + SNAF_SPARE_SIZE(chip->_spi_nand_info));
	}

	_ecc_buf   = dma_alloc_coherent(NULL, (sizeof(*_ecc_buf))*MAX_ECC_BUF_SIZE + 32, &_ecc_buf_addr,GFP_KERNEL);
	_page_buf  = dma_alloc_coherent(NULL, (sizeof(*_page_buf))*RTK_NAND_PAGE_BUF_SIZE + 32, &_page_buf_addr,GFP_KERNEL);
	_bbt       = kmalloc (SNAF_NUM_OF_BLOCK(chip->_spi_nand_info), GFP_KERNEL);
	_bbt_table = kmalloc(bbt_table_len, GFP_KERNEL);

	chip->_ecc_buf   = (__typeof__(*_ecc_buf)*)(((uintptr_t)_ecc_buf+31) & ~ (uintptr_t)0x1F);
	chip->_page_buf  = (__typeof__(*_page_buf)*)(((uintptr_t)_page_buf+31) & ~ (uintptr_t)0x1F);
	chip->_bbt       = (u8_t*)_bbt;
	chip->_bbt_table = (u32_t*)_bbt_table;


#if USE_SECOND_BBT
	_bbt_table2            = kmalloc(bbt_table_len, GFP_KERNEL);
	chip->_bbt_table2      = _bbt_table2;
	if (!_bbt_table2 ) {
        return -ENOMEM;
	} else {
		memset(_bbt_table2, '\0', bbt_table_len);
	}
#endif

	if (!_ecc_buf || !_page_buf || !_bbt_table || !_bbt) {
		printk ("%s: Error, no enough memory for buffer\n", __FUNCTION__);
		return -ENOMEM;
	} else {
		memset(_bbt_table, '\0', bbt_table_len);
		//chip->bbt default all 0.
		memset(chip->_bbt, 0x0, SNAF_NUM_OF_BLOCK(chip->_spi_nand_info));
		return 0;
	}
}

static void
rtk_mtd_remove_buffer(void)
{
	if (_ecc_buf) {
		kfree(_ecc_buf);
		_ecc_buf = NULL;
	}
	if (_page_buf) {
		kfree(_page_buf);
		_page_buf = NULL;
	}
	if (_bbt_table) {
		kfree(_bbt_table);
		_bbt_table = NULL;
	}
#if USE_SECOND_BBT
	if (_bbt_table2) {
		kfree(_bbt_table2);
		_bbt_table2 = NULL;
	}
#endif
}

static void
rtk_display_version(void)
{
	const __u8 *revision;
	const __u8 *date;
	char *running = (__u8 *)VERSION;
	strsep (&running, " ");
	strsep (&running, " ");
	revision = strsep (&running, " ");
	date = strsep (&running, " ");
	printk (BANNER " Rev:%s (%s)\n", revision, date);
}


static int
rtk_read_proc_nandinfo(struct seq_file *m, void *v)
{
	struct luna_nand_t *chip = (struct luna_nand_t *)rtk_mtd->priv;

	seq_printf(m, "%-15s: %u\n", "nand_size"   , (SNAF_NUM_OF_BLOCK(chip->_spi_nand_info)*SNAF_NUM_OF_PAGE_PER_BLK(chip->_spi_nand_info)*SNAF_PAGE_SIZE(chip->_spi_nand_info)));
	seq_printf(m, "%-15s: %u\n", "chip_size"   , (SNAF_NUM_OF_BLOCK(chip->_spi_nand_info)*SNAF_NUM_OF_PAGE_PER_BLK(chip->_spi_nand_info)*SNAF_PAGE_SIZE(chip->_spi_nand_info))>>20);
	seq_printf(m, "%-15s: %u\n" , "block_size"  , rtk_mtd->erasesize);
	seq_printf(m, "%-15s: %u\n" , "chunk_size"  , rtk_mtd->writesize);
	seq_printf(m, "%-15s: %u\n" , "oob_size"    , rtk_mtd->oobsize);
	seq_printf(m, "%-15s: %u\n" , "oob_avail"   , rtk_mtd->oobavail);
	seq_printf(m, "%-15s: %u\n" , "PagePerBlock", SNAF_NUM_OF_PAGE_PER_BLK(chip->_spi_nand_info));

    return 0;
}


static int
rtk_open_proc_nandinfo(struct inode *inode, struct file *file)
{
	return single_open(file, rtk_read_proc_nandinfo, NULL);
}

static const struct file_operations rtk_nand_proc_ops = {
	.open		= rtk_open_proc_nandinfo,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};


extern int g_ecc_select;
static int
rtk_snaf_proc_w(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int flag = 0;
	char tmpbuf[100];
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		if (0 != sscanf(tmpbuf, "%d", &flag)) {
			if (flag) {
				g_ecc_select = SNAF_HARDWARE_ECC;
			} else {
				g_ecc_select = SNAF_SOFTWARE_ECC;
			}
		}
	}
	return count;
}

static int
rtk_snaf_proc_r(struct seq_file *m, void *v)
{
	seq_printf(m, "SNAF ECC select = %d (%s)\n", g_ecc_select, g_ecc_select == 1 ? "Hardware" : "Software");
	return 0;
}

static int
rtk_snaf_open_proc_hw_ecc(struct inode *inode, struct file *file){
	return single_open(file, rtk_snaf_proc_r, NULL);
}

static const struct file_operations _snaf_proc_ops = {
	.write   = rtk_snaf_proc_w,
	.open    = rtk_snaf_open_proc_hw_ecc,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};


static struct proc_dir_entry *_snaf_dir = NULL;
static void
rtk_create_snaf_proc_file(void)
{
	struct proc_dir_entry *proc_file1;
	_snaf_dir = proc_mkdir("spi_nand", NULL);

	proc_file1 = proc_create("hw_ecc", 0644, _snaf_dir, &_snaf_proc_ops);
	if (proc_file1 == NULL) {
		printk("can't create proc: hw_ecc\r\n");
	}

}



#define MTDSIZE	(sizeof (struct mtd_info) + sizeof (struct luna_nand_t))
static int __init
rtk_spi_nand_init (void)
{
	struct nand_chip *this = NULL;
	int rc = 0;

	rtk_display_version();

	rtk_mtd = kmalloc (MTDSIZE, GFP_KERNEL); //mtd_info struct + nand_chip struct
	if (!rtk_mtd) {
		printk ("%s: Error, no enough memory for rtk_mtd\n", __FUNCTION__);
		rc = -ENOMEM;
		goto EXIT;
	}
	memset ((char *)rtk_mtd, 0, MTDSIZE);
	rtk_mtd->priv = this = (struct nand_chip *)(rtk_mtd + 1);
	rtk_mtd->name = "spinand";


	/* init spi nand flash */
	if (rtk_spi_nand_profile () < 0) {
		rc = -1;
		goto EXIT;
	}
	proc_create("nandinfo", 0, NULL, &rtk_nand_proc_ops);
	rtk_create_snaf_proc_file();


EXIT:
	if (rc < 0) {
		if (rtk_mtd) {
			del_mtd_partitions (rtk_mtd);
			kfree(rtk_mtd);
			rtk_mtd = NULL;
		}
		rtk_mtd_remove_buffer();
		remove_proc_entry("nandinfo", NULL);
		remove_proc_entry("hw_ecc", _snaf_dir);
		remove_proc_entry("spi_nand", NULL);

	} else {
		printk (KERN_INFO "Realtek SPI Nand Flash Driver is successfully installing.\n");
	}
	return rc;

}

static void __exit
rtk_spi_nand_exit (void)
{
	if (rtk_mtd) {
		del_mtd_partitions (rtk_mtd);
		kfree (rtk_mtd);
	}
	rtk_mtd_remove_buffer();
	remove_proc_entry("nandinfo", NULL);
	remove_proc_entry("hw_ecc", _snaf_dir);
	remove_proc_entry("spi_nand", NULL);
}
module_init(rtk_spi_nand_init);
module_exit(rtk_spi_nand_exit);
MODULE_AUTHOR("CYYang<chaoyuan@realtek.com>");
MODULE_DESCRIPTION("Realtek Luna SPI NAND Flash Controller Driver");

