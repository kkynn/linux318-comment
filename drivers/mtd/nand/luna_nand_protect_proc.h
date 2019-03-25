/*
 * =====================================================================================
 *
 *       Filename:   luna_nand_protect_proc.h
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



#include <linux/proc_fs.h>
#ifndef LUNA_NAND_PROTECT_PROC_H__
#define LUNA_NAND_PROTECT_PROC_H__


#define LUNA_NAND_PROTECTED_BLOCK_DEFAULT_TOP 5
#define LUNA_NAND_PROTECTED_NAME "nand_protected"

const struct file_operations *get_nand_protect_proc(void);
uint32_t luna_nand_get_blk_protection_top(void);
void luna_nand_set_blk_protection_top(uint32_t new_top_value);


#endif
