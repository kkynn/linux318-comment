/*
 * Copyright (c) 2011, Realtek Semiconductor Corp.
 *
 * file_name: wmpu.h
 *
 *   API of WMPU.
 *
 * Jethro Hsu (jethro@realtek.com)
 * Jul. 15, 2011
 */

#ifndef _ASM_WMPU_H
#define _ASM_WMPU_H

#include <linux/bitops.h>

#include <asm/rlxregs.h>

#define WMPU_FAIL 0
#define WMPU_PASS 1

/*WATCHLO attribute*/
#define ATTR_DW	0x1
#define ATTR_DR	0x2
#define ATTR_IX	0x4

/*WATCHHI attribute*/
#define ASID_ENABLE	0x1
#define ASID_DISABLE	0x0

/*WATCHCTL attribute*/
#define KERNEL_ENALBE	0x1
#define KERNEL_DISABLE	0x0
#define MODE_MP		0x1
#define MODE_WP		0x0
//#define ENTRY_4_EN	0x10 /* only 4 watchpoint register used for WMPU */
#define ENTRY_5_EN	0x20
#define ENTRY_6_EN	0x40
#define ENTRY_7_EN	0x80
/*WMPSTATUS*/
#define ENTRY_MATCH	0x00ff0000

/*field shift*/
#define WMPLO_ADDR	3
#define WMPHI_ASID	16
#define WMPHI_ASID_EN	30
#define WMPCTL_KE	1
#define WMPCTL_EE	16

struct wmpu_lohi_cfg {
        int cfg_num;
        unsigned int wmpulo_addr;
        unsigned char wmpulo_attr;
        unsigned int wmpuhi_mask;
        unsigned char wmpuhi_asid;
        unsigned char wmpuhi_asid_en;
};

struct wmpu_ctl_cfg {
        unsigned char entry_en;
        unsigned char kernel_en;
        unsigned char mode;
};

struct wmpu_info {
        unsigned int wmpuvaddr;
        unsigned char entry_match;
        unsigned char attr_match;
};

typedef void (* callback) (unsigned int addr, unsigned char entry, unsigned char attr);
extern callback wmpu_handler;

struct wmpu_lohi_cfg * rlx_wmpulohi_cfg_alloc(void);
struct wmpu_ctl_cfg * rlx_wmpuctl_cfg_alloc(void);
void rlx_set_wmpu_reg(struct wmpu_lohi_cfg *);
void rlx_set_wmpuctl_reg(struct wmpu_ctl_cfg *);
struct wmpu_info * rlx_wmpu_info(void);
void rlx_wmpulohi_cfg_free(struct wmpu_lohi_cfg *);
void rlxi_wmpu_ctl_free(struct wmpu_ctl_cfg *);
void rlx_wmpu_info_free(struct wmpu_info *);
int rlx_set_wmpu_addr(unsigned int addr, unsigned int mask, unsigned char attr, int reg_num, unsigned char asid, unsigned char asid_en);
int rlx_set_wmpu_ctl(unsigned char entry_enable, unsigned char kernel_enable, unsigned char mode);
void rlx_get_wmpu_info(unsigned int *addr, unsigned char *entry_match, unsigned char *attr_match);
void rlx_wmpu_reset(void);
void rlx_install_wmpu_handler(callback handler);
#endif /* _ASM_WMPU_H */
