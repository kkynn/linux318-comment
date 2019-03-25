/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

/*Image2HeaderVersion: R3 1.0*/
#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8198F_SUPPORT == 1)
static boolean
check_positive(
	struct dm_struct *dm,
	const u32	condition1,
	const u32	condition2,
	const u32	condition3,
	const u32	condition4
)
{
	u32	cond1 = condition1, cond2 = condition2, cond3 = condition3, cond4 = condition4;

	u8	cut_version_for_para = (dm->cut_version ==  ODM_CUT_A) ? 15 : dm->cut_version;
	u8	pkg_type_for_para = (dm->package_type == 0) ? 15 : dm->package_type;

	u32	driver1 = cut_version_for_para << 24 |
			(dm->support_interface & 0xF0) << 16 |
			dm->support_platform << 16 |
			pkg_type_for_para << 12 |
			(dm->support_interface & 0x0F) << 8  |
			dm->rfe_type;

	u32	driver2 = (dm->type_glna & 0xFF) <<  0 |
			(dm->type_gpa & 0xFF)  <<  8 |
			(dm->type_alna & 0xFF) << 16 |
			(dm->type_apa & 0xFF)  << 24;

	u32	driver3 = 0;

	u32	driver4 = (dm->type_glna & 0xFF00) >>  8 |
			(dm->type_gpa & 0xFF00) |
			(dm->type_alna & 0xFF00) << 8 |
			(dm->type_apa & 0xFF00)  << 16;

	PHYDM_DBG(dm, ODM_COMP_INIT,
	"===> check_positive (cond1, cond2, cond3, cond4) = (0x%X 0x%X 0x%X 0x%X)\n", cond1, cond2, cond3, cond4);
	PHYDM_DBG(dm, ODM_COMP_INIT,
	"===> check_positive (driver1, driver2, driver3, driver4) = (0x%X 0x%X 0x%X 0x%X)\n", driver1, driver2, driver3, driver4);

	PHYDM_DBG(dm, ODM_COMP_INIT,
	"	(Platform, Interface) = (0x%X, 0x%X)\n", dm->support_platform, dm->support_interface);
	PHYDM_DBG(dm, ODM_COMP_INIT,
	"	(RFE, Package) = (0x%X, 0x%X)\n", dm->rfe_type, dm->package_type);


	/*============== value Defined Check ===============*/
	/*cut version [27:24] need to do value check*/
	if (((cond1 & 0x0F000000) != 0) && ((cond1 & 0x0F000000) != (driver1 & 0x0F000000)))
		return false;

	/*pkg type [15:12] need to do value check*/
	if (((cond1 & 0x0000F000) != 0) && ((cond1 & 0x0000F000) != (driver1 & 0x0000F000)))
		return false;

	/*interface [11:8] need to do value check*/
	if (((cond1 & 0x00000F00) != 0) && ((cond1 & 0x00000F00) != (driver1 & 0x00000F00)))
		return false;
	/*=============== Bit Defined Check ================*/
	/* We don't care [31:28] */

	cond1 &= 0x000000FF;
	driver1 &= 0x000000FF;

	if (cond1 == driver1)
		return true;
	else
		return false;
}
static boolean
check_negative(
	struct dm_struct *dm,
	const u32	condition1,
	const u32	condition2
)
{
	return true;
}

/******************************************************************************
*                           radioa.TXT
******************************************************************************/

u32 array_mp_8198f_radioa[] = {
		0x000, 0x00030000,
		0x018, 0x0000FC07,
		0x081, 0x0000FC00,
		0x082, 0x000003C0,
		0x084, 0x00000005,
		0x086, 0x000A33A5,
		0x087, 0x00000000,
		0x088, 0x0007B010,
		0x08E, 0x00064540,
		0x08F, 0x000A82D8,
		0x051, 0x00002C06,
		0x052, 0x0007A000,
		0x053, 0x00010061,
		0x054, 0x00060010,
		0x055, 0x00082010,
		0x056, 0x00030CC6,
		0x057, 0x00026C00,
		0x058, 0x00000000,
		0x05A, 0x00050000,
		0x05B, 0x00000006,
		0x05C, 0x00000015,
		0x065, 0x00020000,
		0x06E, 0x00038319,
		0x0EF, 0x00000002,
		0x033, 0x00000301,
		0x033, 0x0001032A,
		0x033, 0x0002032A,
		0x0EF, 0x00000000,
		0x035, 0x00005000,
		0x0F0, 0x00008008,
		0x0EF, 0x00000800,
		0x033, 0x0000002E,
		0x033, 0x00004845,
		0x033, 0x00008848,
		0x033, 0x0000C84B,
		0x033, 0x0001088A,
		0x033, 0x00014CC8,
		0x033, 0x00018CCB,
		0x033, 0x0001CCCE,
		0x033, 0x00020CD1,
		0x033, 0x00024CD4,
		0x033, 0x00028CD7,
		0x033, 0x0004002B,
		0x033, 0x0004402E,
		0x033, 0x00048846,
		0x033, 0x0004C849,
		0x033, 0x00050888,
		0x033, 0x00054CC6,
		0x033, 0x00058CC9,
		0x033, 0x0005CCCC,
		0x033, 0x00060CCF,
		0x033, 0x00064CD2,
		0x033, 0x00068CD5,
		0x0EF, 0x00000000,
		0x0EF, 0x00000400,
		0x033, 0x00001813,
		0x033, 0x00005813,
		0x033, 0x00009193,
		0x033, 0x0000D193,
		0x033, 0x00011D13,
		0x033, 0x00015D13,
		0x033, 0x00019513,
		0x033, 0x0001D513,
		0x0EF, 0x00000000,
		0x0EF, 0x00000200,
		0x033, 0x00000030,
		0x033, 0x00004030,
		0x033, 0x00008030,
		0x033, 0x0000C030,
		0x033, 0x00010030,
		0x033, 0x00014030,
		0x033, 0x00018030,
		0x033, 0x0001C030,
		0x033, 0x00020030,
		0x033, 0x00024030,
		0x033, 0x00028030,
		0x033, 0x0002C030,
		0x033, 0x00030030,
		0x033, 0x00034030,
		0x033, 0x00038030,
		0x033, 0x0003C030,
		0x0EF, 0x00000000,
		0x0EF, 0x00000100,
		0x033, 0x00044000,
		0x033, 0x00048000,
		0x033, 0x0004C000,
		0x033, 0x00050000,
		0x033, 0x00054000,
		0x033, 0x00058000,
		0x033, 0x0005C000,
		0x033, 0x00060000,
		0x033, 0x00064000,
		0x033, 0x00068000,
		0x033, 0x0006C000,
		0x033, 0x00070000,
		0x033, 0x00074000,
		0x033, 0x00078000,
		0x033, 0x00004000,
		0x033, 0x00008000,
		0x033, 0x0000C000,
		0x033, 0x00010000,
		0x033, 0x00014000,
		0x033, 0x00018000,
		0x033, 0x0001C000,
		0x033, 0x00020000,
		0x033, 0x00024002,
		0x033, 0x00028000,
		0x033, 0x0002C000,
		0x033, 0x00030000,
		0x033, 0x00034000,
		0x033, 0x00038000,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080010,
		0x030, 0x00020000,
		0x031, 0x0000006F,
		0x032, 0x00001FF7,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00030000,
		0x031, 0x0000006F,
		0x032, 0x000F1FF3,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00038000,
		0x031, 0x0000006F,
		0x032, 0x000F1DF2,
		0x0EF, 0x00000000,
		0x01B, 0x000746CE,
		0x0EF, 0x00020000,
		0x033, 0x00030000,
		0x033, 0x00038000,
		0x033, 0x00070000,
		0x033, 0x00078000,
		0x0EF, 0x00000000,
		0x0C9, 0x00000F00,
		0x0CA, 0x00002000,
		0x0B0, 0x000FFBC3,
		0x0B1, 0x00033B8F,
		0x0B2, 0x00033762,
		0x0B3, 0x00006000,
		0x0B4, 0x000141F0,
		0x0B5, 0x00014000,
		0x0B6, 0x00012425,
		0x0B7, 0x00010DF0,
		0x0B8, 0x00070FF0,
		0x0B9, 0x000C0008,
		0x0BA, 0x00040005,
		0x0C2, 0x00002C01,
		0x0C3, 0x0000000B,
		0x0C4, 0x00081E2F,
		0x0C5, 0x0005C28F,
		0x0C6, 0x000000A0,
		0x0EF, 0x00008000,
		0x033, 0x00001524,
		0x034, 0x0001FF83,
		0x035, 0x000053FF,
		0x0EF, 0x00000000,
		0xFFE, 0x00000000,
		0x018, 0x00008C07,
		0xFFE, 0x00000000,
		0xFFE, 0x00000000,
		0xFFE, 0x00000000,
		0x000, 0x00031DD5,

};

void
odm_read_and_config_mp_8198f_radioa(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_radioa)/sizeof(u32);
	u32	*array = array_mp_8198f_radioa;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_radioa\n");

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29)|BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped?false:true;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ELSE\n");
				} else {/*if , else if*/
					pre_v1 = v1;
					pre_v2 = v2;
					PHYDM_DBG(dm, ODM_COMP_INIT, "IF or ELSE IF\n");
				}
			} else if (v1 & BIT(30)) { /*negative condition*/
				if (is_skipped == false) {
					if (check_positive(dm, pre_v1, pre_v2, v1, v2)) {
						is_matched = true;
						is_skipped = true;
					} else {
						is_matched = false;
						is_skipped = false;
					}
				} else
					is_matched = false;
			}
		} else {
			if (is_matched)
				odm_config_rf_radio_a_8198f(dm, v1, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_radioa(void)
{
		return 4;
}

/******************************************************************************
*                           radiob.TXT
******************************************************************************/

u32 array_mp_8198f_radiob[] = {
		0x000, 0x00030000,
		0x081, 0x0000FC00,
		0x082, 0x000003C0,
		0x084, 0x00000005,
		0x086, 0x000A33A5,
		0x087, 0x00000000,
		0x088, 0x0007B010,
		0x08E, 0x00064540,
		0x08F, 0x000A82D8,
		0x051, 0x00002C06,
		0x052, 0x0007A007,
		0x053, 0x00010061,
		0x054, 0x00060010,
		0x055, 0x00082010,
		0x056, 0x00030CC6,
		0x057, 0x00037C00,
		0x058, 0x00000000,
		0x05A, 0x00050000,
		0x05B, 0x00000006,
		0x05C, 0x00000015,
		0x065, 0x00020000,
		0x06E, 0x00038319,
		0x0EF, 0x00000002,
		0x033, 0x00000301,
		0x033, 0x0001032A,
		0x033, 0x0002032A,
		0x0EF, 0x00000000,
		0x035, 0x00005000,
		0x0F0, 0x00008008,
		0x0EF, 0x00000800,
		0x033, 0x0000002E,
		0x033, 0x00004845,
		0x033, 0x00008848,
		0x033, 0x0000C84B,
		0x033, 0x0001088A,
		0x033, 0x00014CC8,
		0x033, 0x00018CCB,
		0x033, 0x0001CCCE,
		0x033, 0x00020CD1,
		0x033, 0x00024CD4,
		0x033, 0x00028CD7,
		0x033, 0x0004002B,
		0x033, 0x0004402E,
		0x033, 0x00048846,
		0x033, 0x0004C849,
		0x033, 0x00050888,
		0x033, 0x00054CC6,
		0x033, 0x00058CC9,
		0x033, 0x0005CCCC,
		0x033, 0x00060CCF,
		0x033, 0x00064CD2,
		0x033, 0x00068CD5,
		0x0EF, 0x00000000,
		0x0EF, 0x00000400,
		0x033, 0x00001B93,
		0x033, 0x00005B93,
		0x033, 0x00009193,
		0x033, 0x0000D193,
		0x033, 0x00011D13,
		0x033, 0x00015D13,
		0x033, 0x00019513,
		0x033, 0x0001D513,
		0x0EF, 0x00000000,
		0x0EF, 0x00000200,
		0x033, 0x00000030,
		0x033, 0x00004030,
		0x033, 0x00008030,
		0x033, 0x0000C030,
		0x033, 0x00010030,
		0x033, 0x00014030,
		0x033, 0x00018030,
		0x033, 0x0001C030,
		0x033, 0x00020030,
		0x033, 0x00024030,
		0x033, 0x00028030,
		0x033, 0x0002C030,
		0x033, 0x00030030,
		0x033, 0x00034030,
		0x033, 0x00038030,
		0x033, 0x0003C030,
		0x0EF, 0x00000000,
		0x0EF, 0x00000100,
		0x033, 0x00044000,
		0x033, 0x00048000,
		0x033, 0x0004C000,
		0x033, 0x00050000,
		0x033, 0x00054000,
		0x033, 0x00058000,
		0x033, 0x0005C000,
		0x033, 0x00060000,
		0x033, 0x00064000,
		0x033, 0x00068000,
		0x033, 0x0006C002,
		0x033, 0x00070000,
		0x033, 0x00074000,
		0x033, 0x00078000,
		0x033, 0x00004000,
		0x033, 0x00008000,
		0x033, 0x0000C000,
		0x033, 0x00010000,
		0x033, 0x00014000,
		0x033, 0x00018000,
		0x033, 0x0001C000,
		0x033, 0x00020000,
		0x033, 0x00024002,
		0x033, 0x00028000,
		0x033, 0x0002C000,
		0x033, 0x00030000,
		0x033, 0x00034000,
		0x033, 0x00038000,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080010,
		0x030, 0x00020000,
		0x031, 0x0000006F,
		0x032, 0x00001FF7,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00030000,
		0x031, 0x0000006F,
		0x032, 0x000F1FF3,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00038000,
		0x031, 0x0000006F,
		0x032, 0x000F1DF2,
		0x0EF, 0x00000000,
		0x01B, 0x000746CE,
		0x0EF, 0x00020000,
		0x033, 0x00030000,
		0x033, 0x00038000,
		0x033, 0x00070000,
		0x033, 0x00078000,
		0x0EF, 0x00000000,
		0x000, 0x00031DD5,

};

void
odm_read_and_config_mp_8198f_radiob(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_radiob)/sizeof(u32);
	u32	*array = array_mp_8198f_radiob;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_radiob\n");

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29)|BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped?false:true;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ELSE\n");
				} else {/*if , else if*/
					pre_v1 = v1;
					pre_v2 = v2;
					PHYDM_DBG(dm, ODM_COMP_INIT, "IF or ELSE IF\n");
				}
			} else if (v1 & BIT(30)) { /*negative condition*/
				if (is_skipped == false) {
					if (check_positive(dm, pre_v1, pre_v2, v1, v2)) {
						is_matched = true;
						is_skipped = true;
					} else {
						is_matched = false;
						is_skipped = false;
					}
				} else
					is_matched = false;
			}
		} else {
			if (is_matched)
				odm_config_rf_radio_b_8198f(dm, v1, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_radiob(void)
{
		return 4;
}

/******************************************************************************
*                           radioc.TXT
******************************************************************************/

u32 array_mp_8198f_radioc[] = {
		0x000, 0x00030000,
		0x081, 0x0000FC00,
		0x082, 0x000003C0,
		0x084, 0x00000005,
		0x086, 0x000A33A5,
		0x087, 0x00000000,
		0x088, 0x0007B010,
		0x08E, 0x00064540,
		0x08F, 0x000A82D8,
		0x051, 0x00002C06,
		0x052, 0x0007A007,
		0x053, 0x00010061,
		0x054, 0x00060010,
		0x055, 0x00082010,
		0x056, 0x00030CC6,
		0x057, 0x00037C00,
		0x058, 0x00000000,
		0x05A, 0x00050000,
		0x05B, 0x00000006,
		0x05C, 0x00000015,
		0x065, 0x00020000,
		0x06E, 0x00038319,
		0x0EF, 0x00000002,
		0x033, 0x00000301,
		0x033, 0x0001032A,
		0x033, 0x0002032A,
		0x0EF, 0x00000000,
		0x035, 0x00005000,
		0x0F0, 0x00008008,
		0x0EF, 0x00000800,
		0x033, 0x0000002E,
		0x033, 0x00004845,
		0x033, 0x00008848,
		0x033, 0x0000C84B,
		0x033, 0x0001088A,
		0x033, 0x00014CC8,
		0x033, 0x00018CCB,
		0x033, 0x0001CCCE,
		0x033, 0x00020CD1,
		0x033, 0x00024CD4,
		0x033, 0x00028CD7,
		0x033, 0x0004002B,
		0x033, 0x0004402E,
		0x033, 0x00048846,
		0x033, 0x0004C849,
		0x033, 0x00050888,
		0x033, 0x00054CC6,
		0x033, 0x00058CC9,
		0x033, 0x0005CCCC,
		0x033, 0x00060CCF,
		0x033, 0x00064CD2,
		0x033, 0x00068CD5,
		0x0EF, 0x00000000,
		0x0EF, 0x00000400,
		0x033, 0x00001B93,
		0x033, 0x00005B93,
		0x033, 0x00009193,
		0x033, 0x0000D193,
		0x033, 0x00011D13,
		0x033, 0x00015D13,
		0x033, 0x00019513,
		0x033, 0x0001D513,
		0x0EF, 0x00000000,
		0x0EF, 0x00000200,
		0x033, 0x00000030,
		0x033, 0x00004030,
		0x033, 0x00008030,
		0x033, 0x0000C030,
		0x033, 0x00010030,
		0x033, 0x00014030,
		0x033, 0x00018030,
		0x033, 0x0001C030,
		0x033, 0x00020030,
		0x033, 0x00024030,
		0x033, 0x00028030,
		0x033, 0x0002C030,
		0x033, 0x00030030,
		0x033, 0x00034030,
		0x033, 0x00038030,
		0x033, 0x0003C030,
		0x0EF, 0x00000000,
		0x0EF, 0x00000100,
		0x033, 0x00044000,
		0x033, 0x00048000,
		0x033, 0x0004C000,
		0x033, 0x00050000,
		0x033, 0x00054000,
		0x033, 0x00058000,
		0x033, 0x0005C000,
		0x033, 0x00060000,
		0x033, 0x00064000,
		0x033, 0x00068000,
		0x033, 0x0006C002,
		0x033, 0x00070000,
		0x033, 0x00074000,
		0x033, 0x00078000,
		0x033, 0x00004000,
		0x033, 0x00008000,
		0x033, 0x0000C000,
		0x033, 0x00010000,
		0x033, 0x00014000,
		0x033, 0x00018000,
		0x033, 0x0001C000,
		0x033, 0x00020000,
		0x033, 0x00024002,
		0x033, 0x00028000,
		0x033, 0x0002C000,
		0x033, 0x00030000,
		0x033, 0x00034000,
		0x033, 0x00038000,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080010,
		0x030, 0x00020000,
		0x031, 0x0000006F,
		0x032, 0x00001FF7,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00030000,
		0x031, 0x0000006F,
		0x032, 0x000F1FF3,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00038000,
		0x031, 0x0000006F,
		0x032, 0x000F1DF2,
		0x0EF, 0x00000000,
		0x01B, 0x000746CE,
		0x0EF, 0x00020000,
		0x033, 0x00030000,
		0x033, 0x00038000,
		0x033, 0x00070000,
		0x033, 0x00078000,
		0x0EF, 0x00000000,
		0x000, 0x00031DD5,

};

void
odm_read_and_config_mp_8198f_radioc(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_radioc)/sizeof(u32);
	u32	*array = array_mp_8198f_radioc;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_radioc\n");

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29)|BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped?false:true;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ELSE\n");
				} else {/*if , else if*/
					pre_v1 = v1;
					pre_v2 = v2;
					PHYDM_DBG(dm, ODM_COMP_INIT, "IF or ELSE IF\n");
				}
			} else if (v1 & BIT(30)) { /*negative condition*/
				if (is_skipped == false) {
					if (check_positive(dm, pre_v1, pre_v2, v1, v2)) {
						is_matched = true;
						is_skipped = true;
					} else {
						is_matched = false;
						is_skipped = false;
					}
				} else
					is_matched = false;
			}
		} else {
			if (is_matched)
				odm_config_rf_radio_c_8198f(dm, v1, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_radioc(void)
{
		return 4;
}

/******************************************************************************
*                           radiod.TXT
******************************************************************************/

u32 array_mp_8198f_radiod[] = {
		0x000, 0x00030000,
		0x081, 0x0000FC00,
		0x082, 0x000003C0,
		0x084, 0x00000005,
		0x086, 0x000A33A5,
		0x087, 0x00000000,
		0x088, 0x0007B010,
		0x08E, 0x00064540,
		0x08F, 0x000A82D8,
		0x051, 0x00002C06,
		0x052, 0x0007A007,
		0x053, 0x00010061,
		0x054, 0x00060010,
		0x055, 0x00082010,
		0x056, 0x00030CC6,
		0x057, 0x00037C00,
		0x058, 0x00000000,
		0x05A, 0x00050000,
		0x05B, 0x00000006,
		0x05C, 0x00000015,
		0x065, 0x00020000,
		0x06E, 0x00038319,
		0x0EF, 0x00000002,
		0x033, 0x00000301,
		0x033, 0x0001032A,
		0x033, 0x0002032A,
		0x0EF, 0x00000000,
		0x035, 0x00005000,
		0x0F0, 0x00008008,
		0x0EF, 0x00000800,
		0x033, 0x0000002E,
		0x033, 0x00004845,
		0x033, 0x00008848,
		0x033, 0x0000C84B,
		0x033, 0x0001088A,
		0x033, 0x00014CC8,
		0x033, 0x00018CCB,
		0x033, 0x0001CCCE,
		0x033, 0x00020CD1,
		0x033, 0x00024CD4,
		0x033, 0x00028CD7,
		0x033, 0x0004002B,
		0x033, 0x0004402E,
		0x033, 0x00048846,
		0x033, 0x0004C849,
		0x033, 0x00050888,
		0x033, 0x00054CC6,
		0x033, 0x00058CC9,
		0x033, 0x0005CCCC,
		0x033, 0x00060CCF,
		0x033, 0x00064CD2,
		0x033, 0x00068CD5,
		0x0EF, 0x00000000,
		0x0EF, 0x00000400,
		0x033, 0x00001B93,
		0x033, 0x00005B93,
		0x033, 0x00009193,
		0x033, 0x0000D193,
		0x033, 0x00011D13,
		0x033, 0x00015D13,
		0x033, 0x00019513,
		0x033, 0x0001D513,
		0x0EF, 0x00000000,
		0x0EF, 0x00000200,
		0x033, 0x00000030,
		0x033, 0x00004030,
		0x033, 0x00008030,
		0x033, 0x0000C030,
		0x033, 0x00010030,
		0x033, 0x00014030,
		0x033, 0x00018030,
		0x033, 0x0001C030,
		0x033, 0x00020030,
		0x033, 0x00024030,
		0x033, 0x00028030,
		0x033, 0x0002C030,
		0x033, 0x00030030,
		0x033, 0x00034030,
		0x033, 0x00038030,
		0x033, 0x0003C030,
		0x0EF, 0x00000000,
		0x0EF, 0x00000100,
		0x033, 0x00044000,
		0x033, 0x00048000,
		0x033, 0x0004C000,
		0x033, 0x00050000,
		0x033, 0x00054000,
		0x033, 0x00058000,
		0x033, 0x0005C000,
		0x033, 0x00060000,
		0x033, 0x00064000,
		0x033, 0x00068000,
		0x033, 0x0006C002,
		0x033, 0x00070000,
		0x033, 0x00074000,
		0x033, 0x00078000,
		0x033, 0x00004000,
		0x033, 0x00008000,
		0x033, 0x0000C000,
		0x033, 0x00010000,
		0x033, 0x00014000,
		0x033, 0x00018000,
		0x033, 0x0001C000,
		0x033, 0x00020000,
		0x033, 0x00024002,
		0x033, 0x00028000,
		0x033, 0x0002C000,
		0x033, 0x00030000,
		0x033, 0x00034000,
		0x033, 0x00038000,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080010,
		0x030, 0x00020000,
		0x031, 0x0000006F,
		0x032, 0x00001FF7,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00030000,
		0x031, 0x0000006F,
		0x032, 0x000F1FF3,
		0x0EF, 0x00000000,
		0x084, 0x00000000,
		0x0EF, 0x00080000,
		0x030, 0x00038000,
		0x031, 0x0000006F,
		0x032, 0x000F1DF2,
		0x0EF, 0x00000000,
		0x01B, 0x000746CE,
		0x0EF, 0x00020000,
		0x033, 0x00030000,
		0x033, 0x00038000,
		0x033, 0x00070000,
		0x033, 0x00078000,
		0x0EF, 0x00000000,
		0x000, 0x00031DD5,

};

void
odm_read_and_config_mp_8198f_radiod(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_radiod)/sizeof(u32);
	u32	*array = array_mp_8198f_radiod;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_radiod\n");

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29)|BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped?false:true;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ELSE\n");
				} else {/*if , else if*/
					pre_v1 = v1;
					pre_v2 = v2;
					PHYDM_DBG(dm, ODM_COMP_INIT, "IF or ELSE IF\n");
				}
			} else if (v1 & BIT(30)) { /*negative condition*/
				if (is_skipped == false) {
					if (check_positive(dm, pre_v1, pre_v2, v1, v2)) {
						is_matched = true;
						is_skipped = true;
					} else {
						is_matched = false;
						is_skipped = false;
					}
				} else
					is_matched = false;
			}
		} else {
			if (is_matched)
				odm_config_rf_radio_d_8198f(dm, v1, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_radiod(void)
{
		return 4;
}

/******************************************************************************
*                           txpowertrack.TXT
******************************************************************************/

u8 delta_swingidx_mp_5gb_n_txpwrtrk_8198f[][DELTA_SWINGIDX_SIZE] = {
	{0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 15, 15, 15},
	{0, 1, 1, 2, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 8, 9, 10, 10, 11, 12, 12, 13, 13, 14, 14, 14, 14, 14, 14, 14},
	{0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 12, 12, 13, 13, 14, 14, 14, 14, 14, 14, 14},
};
u8 delta_swingidx_mp_5gb_p_txpwrtrk_8198f[][DELTA_SWINGIDX_SIZE] = {
	{0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 8, 9, 10, 11, 11, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 19, 19, 19, 19, 19},
	{0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 17, 18, 18, 18, 18, 18, 18},
	{0, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 10, 11, 12, 13, 14, 15, 15, 16, 16, 17, 17, 17, 17, 17, 17},
};
u8 delta_swingidx_mp_5ga_n_txpwrtrk_8198f[][DELTA_SWINGIDX_SIZE] = {
	{0, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 15, 15, 15},
	{0, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 14, 14, 14, 14, 14},
	{0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 13, 14, 14, 14, 14, 14, 14, 14},
};
u8 delta_swingidx_mp_5ga_p_txpwrtrk_8198f[][DELTA_SWINGIDX_SIZE] = {
	{0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 8, 9, 9, 10, 11, 12, 13, 14, 14, 15, 16, 17, 18, 19, 19, 20, 20, 20, 20, 20},
	{0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 14, 15, 16, 16, 17, 17, 18, 18, 18, 18},
	{0, 1, 2, 3, 3, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 17, 18, 18, 18, 18, 18},
};
u8 delta_swingidx_mp_2gb_n_txpwrtrk_8198f[]    = {0, 1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 16, 17, 18, 18, 18, 18, 18, 18, 18, 18};
u8 delta_swingidx_mp_2gb_p_txpwrtrk_8198f[]    = {0, 1, 1, 2, 3, 4, 4, 5, 6, 7, 7, 8, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 22, 22, 22};
u8 delta_swingidx_mp_2ga_n_txpwrtrk_8198f[]    = {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18};
u8 delta_swingidx_mp_2ga_p_txpwrtrk_8198f[]    = {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 22, 22};
u8 delta_swingidx_mp_2g_cck_b_n_txpwrtrk_8198f[] = {0, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17};
u8 delta_swingidx_mp_2g_cck_b_p_txpwrtrk_8198f[] = {0, 1, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 22, 22};
u8 delta_swingidx_mp_2g_cck_a_n_txpwrtrk_8198f[] = {0, 1, 2, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18};
u8 delta_swingidx_mp_2g_cck_a_p_txpwrtrk_8198f[] = {0, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 22, 22};

void
odm_read_and_config_mp_8198f_txpowertrack(
	struct dm_struct	 *dm
)
{
	struct dm_rf_calibration_struct  *cali_info = &dm->rf_calibrate_info;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> ODM_ReadAndConfig_MP_mp_8198f\n");


	odm_move_memory(dm, cali_info->delta_swing_table_idx_2ga_p, delta_swingidx_mp_2ga_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2ga_n, delta_swingidx_mp_2ga_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2gb_p, delta_swingidx_mp_2gb_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2gb_n, delta_swingidx_mp_2gb_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);

	odm_move_memory(dm, cali_info->delta_swing_table_idx_2g_cck_a_p, delta_swingidx_mp_2g_cck_a_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2g_cck_a_n, delta_swingidx_mp_2g_cck_a_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2g_cck_b_p, delta_swingidx_mp_2g_cck_b_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_2g_cck_b_n, delta_swingidx_mp_2g_cck_b_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE);

	odm_move_memory(dm, cali_info->delta_swing_table_idx_5ga_p, delta_swingidx_mp_5ga_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE*3);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_5ga_n, delta_swingidx_mp_5ga_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE*3);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_5gb_p, delta_swingidx_mp_5gb_p_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE*3);
	odm_move_memory(dm, cali_info->delta_swing_table_idx_5gb_n, delta_swingidx_mp_5gb_n_txpwrtrk_8198f, DELTA_SWINGIDX_SIZE*3);
}

/******************************************************************************
*                           txpwr_lmt.TXT
******************************************************************************/

const char *array_mp_8198f_txpwr_lmt[] = {
	"FCC", "2.4G", "20M", "CCK", "1T", "01", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "01", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "01", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "02", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "02", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "02", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "03", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "03", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "03", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "04", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "04", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "04", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "05", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "05", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "05", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "06", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "06", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "06", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "07", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "07", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "07", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "08", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "08", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "08", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "09", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "09", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "09", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "10", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "10", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "10", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "11", "32",
	"ETSI", "2.4G", "20M", "CCK", "1T", "11", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "11", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "12", "26",
	"ETSI", "2.4G", "20M", "CCK", "1T", "12", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "12", "30",
	"FCC", "2.4G", "20M", "CCK", "1T", "13", "20",
	"ETSI", "2.4G", "20M", "CCK", "1T", "13", "28",
	"MKK", "2.4G", "20M", "CCK", "1T", "13", "28",
	"FCC", "2.4G", "20M", "CCK", "1T", "14", "63",
	"ETSI", "2.4G", "20M", "CCK", "1T", "14", "63",
	"MKK", "2.4G", "20M", "CCK", "1T", "14", "32",
	"FCC", "2.4G", "20M", "OFDM", "1T", "01", "26",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "01", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "01", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "02", "30",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "02", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "02", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "03", "32",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "03", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "03", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "04", "34",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "04", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "04", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "05", "34",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "05", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "05", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "06", "34",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "06", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "06", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "07", "34",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "07", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "07", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "08", "34",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "08", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "08", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "09", "32",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "09", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "09", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "10", "30",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "10", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "10", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "11", "28",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "11", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "11", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "12", "22",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "12", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "12", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "13", "14",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "13", "30",
	"MKK", "2.4G", "20M", "OFDM", "1T", "13", "34",
	"FCC", "2.4G", "20M", "OFDM", "1T", "14", "63",
	"ETSI", "2.4G", "20M", "OFDM", "1T", "14", "63",
	"MKK", "2.4G", "20M", "OFDM", "1T", "14", "63",
	"FCC", "2.4G", "20M", "HT", "1T", "01", "26",
	"ETSI", "2.4G", "20M", "HT", "1T", "01", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "01", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "02", "30",
	"ETSI", "2.4G", "20M", "HT", "1T", "02", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "02", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "03", "32",
	"ETSI", "2.4G", "20M", "HT", "1T", "03", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "03", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "04", "34",
	"ETSI", "2.4G", "20M", "HT", "1T", "04", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "04", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "05", "34",
	"ETSI", "2.4G", "20M", "HT", "1T", "05", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "05", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "06", "34",
	"ETSI", "2.4G", "20M", "HT", "1T", "06", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "06", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "07", "34",
	"ETSI", "2.4G", "20M", "HT", "1T", "07", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "07", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "08", "34",
	"ETSI", "2.4G", "20M", "HT", "1T", "08", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "08", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "09", "32",
	"ETSI", "2.4G", "20M", "HT", "1T", "09", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "09", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "10", "30",
	"ETSI", "2.4G", "20M", "HT", "1T", "10", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "10", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "11", "26",
	"ETSI", "2.4G", "20M", "HT", "1T", "11", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "11", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "12", "20",
	"ETSI", "2.4G", "20M", "HT", "1T", "12", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "12", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "13", "14",
	"ETSI", "2.4G", "20M", "HT", "1T", "13", "30",
	"MKK", "2.4G", "20M", "HT", "1T", "13", "34",
	"FCC", "2.4G", "20M", "HT", "1T", "14", "63",
	"ETSI", "2.4G", "20M", "HT", "1T", "14", "63",
	"MKK", "2.4G", "20M", "HT", "1T", "14", "63",
	"FCC", "2.4G", "20M", "HT", "2T", "01", "26",
	"ETSI", "2.4G", "20M", "HT", "2T", "01", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "01", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "02", "28",
	"ETSI", "2.4G", "20M", "HT", "2T", "02", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "02", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "03", "30",
	"ETSI", "2.4G", "20M", "HT", "2T", "03", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "03", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "04", "30",
	"ETSI", "2.4G", "20M", "HT", "2T", "04", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "04", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "05", "32",
	"ETSI", "2.4G", "20M", "HT", "2T", "05", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "05", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "06", "32",
	"ETSI", "2.4G", "20M", "HT", "2T", "06", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "06", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "07", "32",
	"ETSI", "2.4G", "20M", "HT", "2T", "07", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "07", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "08", "30",
	"ETSI", "2.4G", "20M", "HT", "2T", "08", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "08", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "09", "30",
	"ETSI", "2.4G", "20M", "HT", "2T", "09", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "09", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "10", "28",
	"ETSI", "2.4G", "20M", "HT", "2T", "10", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "10", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "11", "26",
	"ETSI", "2.4G", "20M", "HT", "2T", "11", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "11", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "12", "20",
	"ETSI", "2.4G", "20M", "HT", "2T", "12", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "12", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "13", "14",
	"ETSI", "2.4G", "20M", "HT", "2T", "13", "18",
	"MKK", "2.4G", "20M", "HT", "2T", "13", "30",
	"FCC", "2.4G", "20M", "HT", "2T", "14", "63",
	"ETSI", "2.4G", "20M", "HT", "2T", "14", "63",
	"MKK", "2.4G", "20M", "HT", "2T", "14", "63",
	"FCC", "2.4G", "40M", "HT", "1T", "01", "63",
	"ETSI", "2.4G", "40M", "HT", "1T", "01", "63",
	"MKK", "2.4G", "40M", "HT", "1T", "01", "63",
	"FCC", "2.4G", "40M", "HT", "1T", "02", "63",
	"ETSI", "2.4G", "40M", "HT", "1T", "02", "63",
	"MKK", "2.4G", "40M", "HT", "1T", "02", "63",
	"FCC", "2.4G", "40M", "HT", "1T", "03", "26",
	"ETSI", "2.4G", "40M", "HT", "1T", "03", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "03", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "04", "26",
	"ETSI", "2.4G", "40M", "HT", "1T", "04", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "04", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "05", "30",
	"ETSI", "2.4G", "40M", "HT", "1T", "05", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "05", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "06", "32",
	"ETSI", "2.4G", "40M", "HT", "1T", "06", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "06", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "07", "30",
	"ETSI", "2.4G", "40M", "HT", "1T", "07", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "07", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "08", "26",
	"ETSI", "2.4G", "40M", "HT", "1T", "08", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "08", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "09", "26",
	"ETSI", "2.4G", "40M", "HT", "1T", "09", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "09", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "10", "20",
	"ETSI", "2.4G", "40M", "HT", "1T", "10", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "10", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "11", "14",
	"ETSI", "2.4G", "40M", "HT", "1T", "11", "30",
	"MKK", "2.4G", "40M", "HT", "1T", "11", "34",
	"FCC", "2.4G", "40M", "HT", "1T", "12", "63",
	"ETSI", "2.4G", "40M", "HT", "1T", "12", "63",
	"MKK", "2.4G", "40M", "HT", "1T", "12", "63",
	"FCC", "2.4G", "40M", "HT", "1T", "13", "63",
	"ETSI", "2.4G", "40M", "HT", "1T", "13", "63",
	"MKK", "2.4G", "40M", "HT", "1T", "13", "63",
	"FCC", "2.4G", "40M", "HT", "1T", "14", "63",
	"ETSI", "2.4G", "40M", "HT", "1T", "14", "63",
	"MKK", "2.4G", "40M", "HT", "1T", "14", "63",
	"FCC", "2.4G", "40M", "HT", "2T", "01", "63",
	"ETSI", "2.4G", "40M", "HT", "2T", "01", "63",
	"MKK", "2.4G", "40M", "HT", "2T", "01", "63",
	"FCC", "2.4G", "40M", "HT", "2T", "02", "63",
	"ETSI", "2.4G", "40M", "HT", "2T", "02", "63",
	"MKK", "2.4G", "40M", "HT", "2T", "02", "63",
	"FCC", "2.4G", "40M", "HT", "2T", "03", "24",
	"ETSI", "2.4G", "40M", "HT", "2T", "03", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "03", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "04", "24",
	"ETSI", "2.4G", "40M", "HT", "2T", "04", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "04", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "05", "26",
	"ETSI", "2.4G", "40M", "HT", "2T", "05", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "05", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "06", "28",
	"ETSI", "2.4G", "40M", "HT", "2T", "06", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "06", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "07", "26",
	"ETSI", "2.4G", "40M", "HT", "2T", "07", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "07", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "08", "26",
	"ETSI", "2.4G", "40M", "HT", "2T", "08", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "08", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "09", "26",
	"ETSI", "2.4G", "40M", "HT", "2T", "09", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "09", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "10", "20",
	"ETSI", "2.4G", "40M", "HT", "2T", "10", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "10", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "11", "14",
	"ETSI", "2.4G", "40M", "HT", "2T", "11", "18",
	"MKK", "2.4G", "40M", "HT", "2T", "11", "30",
	"FCC", "2.4G", "40M", "HT", "2T", "12", "63",
	"ETSI", "2.4G", "40M", "HT", "2T", "12", "63",
	"MKK", "2.4G", "40M", "HT", "2T", "12", "63",
	"FCC", "2.4G", "40M", "HT", "2T", "13", "63",
	"ETSI", "2.4G", "40M", "HT", "2T", "13", "63",
	"MKK", "2.4G", "40M", "HT", "2T", "13", "63",
	"FCC", "2.4G", "40M", "HT", "2T", "14", "63",
	"ETSI", "2.4G", "40M", "HT", "2T", "14", "63",
	"MKK", "2.4G", "40M", "HT", "2T", "14", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "36", "30",
	"ETSI", "5G", "20M", "OFDM", "1T", "36", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "36", "30",
	"FCC", "5G", "20M", "OFDM", "1T", "40", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "40", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "40", "30",
	"FCC", "5G", "20M", "OFDM", "1T", "44", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "44", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "44", "30",
	"FCC", "5G", "20M", "OFDM", "1T", "48", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "48", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "48", "30",
	"FCC", "5G", "20M", "OFDM", "1T", "52", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "52", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "52", "28",
	"FCC", "5G", "20M", "OFDM", "1T", "56", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "56", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "56", "28",
	"FCC", "5G", "20M", "OFDM", "1T", "60", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "60", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "60", "28",
	"FCC", "5G", "20M", "OFDM", "1T", "64", "28",
	"ETSI", "5G", "20M", "OFDM", "1T", "64", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "64", "28",
	"FCC", "5G", "20M", "OFDM", "1T", "100", "26",
	"ETSI", "5G", "20M", "OFDM", "1T", "100", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "100", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "104", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "104", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "104", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "108", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "108", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "108", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "112", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "112", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "112", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "116", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "116", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "116", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "120", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "120", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "120", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "124", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "124", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "124", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "128", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "128", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "128", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "132", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "132", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "132", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "136", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "136", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "136", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "140", "28",
	"ETSI", "5G", "20M", "OFDM", "1T", "140", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "140", "32",
	"FCC", "5G", "20M", "OFDM", "1T", "144", "28",
	"ETSI", "5G", "20M", "OFDM", "1T", "144", "32",
	"MKK", "5G", "20M", "OFDM", "1T", "144", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "149", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "149", "63",
	"MKK", "5G", "20M", "OFDM", "1T", "149", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "153", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "153", "63",
	"MKK", "5G", "20M", "OFDM", "1T", "153", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "157", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "157", "63",
	"MKK", "5G", "20M", "OFDM", "1T", "157", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "161", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "161", "63",
	"MKK", "5G", "20M", "OFDM", "1T", "161", "63",
	"FCC", "5G", "20M", "OFDM", "1T", "165", "32",
	"ETSI", "5G", "20M", "OFDM", "1T", "165", "63",
	"MKK", "5G", "20M", "OFDM", "1T", "165", "63",
	"FCC", "5G", "20M", "HT", "1T", "36", "30",
	"ETSI", "5G", "20M", "HT", "1T", "36", "32",
	"MKK", "5G", "20M", "HT", "1T", "36", "28",
	"FCC", "5G", "20M", "HT", "1T", "40", "32",
	"ETSI", "5G", "20M", "HT", "1T", "40", "32",
	"MKK", "5G", "20M", "HT", "1T", "40", "28",
	"FCC", "5G", "20M", "HT", "1T", "44", "32",
	"ETSI", "5G", "20M", "HT", "1T", "44", "32",
	"MKK", "5G", "20M", "HT", "1T", "44", "28",
	"FCC", "5G", "20M", "HT", "1T", "48", "32",
	"ETSI", "5G", "20M", "HT", "1T", "48", "32",
	"MKK", "5G", "20M", "HT", "1T", "48", "28",
	"FCC", "5G", "20M", "HT", "1T", "52", "32",
	"ETSI", "5G", "20M", "HT", "1T", "52", "32",
	"MKK", "5G", "20M", "HT", "1T", "52", "28",
	"FCC", "5G", "20M", "HT", "1T", "56", "32",
	"ETSI", "5G", "20M", "HT", "1T", "56", "32",
	"MKK", "5G", "20M", "HT", "1T", "56", "28",
	"FCC", "5G", "20M", "HT", "1T", "60", "32",
	"ETSI", "5G", "20M", "HT", "1T", "60", "32",
	"MKK", "5G", "20M", "HT", "1T", "60", "28",
	"FCC", "5G", "20M", "HT", "1T", "64", "28",
	"ETSI", "5G", "20M", "HT", "1T", "64", "32",
	"MKK", "5G", "20M", "HT", "1T", "64", "28",
	"FCC", "5G", "20M", "HT", "1T", "100", "26",
	"ETSI", "5G", "20M", "HT", "1T", "100", "32",
	"MKK", "5G", "20M", "HT", "1T", "100", "32",
	"FCC", "5G", "20M", "HT", "1T", "104", "32",
	"ETSI", "5G", "20M", "HT", "1T", "104", "32",
	"MKK", "5G", "20M", "HT", "1T", "104", "32",
	"FCC", "5G", "20M", "HT", "1T", "108", "32",
	"ETSI", "5G", "20M", "HT", "1T", "108", "32",
	"MKK", "5G", "20M", "HT", "1T", "108", "32",
	"FCC", "5G", "20M", "HT", "1T", "112", "32",
	"ETSI", "5G", "20M", "HT", "1T", "112", "32",
	"MKK", "5G", "20M", "HT", "1T", "112", "32",
	"FCC", "5G", "20M", "HT", "1T", "116", "32",
	"ETSI", "5G", "20M", "HT", "1T", "116", "32",
	"MKK", "5G", "20M", "HT", "1T", "116", "32",
	"FCC", "5G", "20M", "HT", "1T", "120", "32",
	"ETSI", "5G", "20M", "HT", "1T", "120", "32",
	"MKK", "5G", "20M", "HT", "1T", "120", "32",
	"FCC", "5G", "20M", "HT", "1T", "124", "32",
	"ETSI", "5G", "20M", "HT", "1T", "124", "32",
	"MKK", "5G", "20M", "HT", "1T", "124", "32",
	"FCC", "5G", "20M", "HT", "1T", "128", "32",
	"ETSI", "5G", "20M", "HT", "1T", "128", "32",
	"MKK", "5G", "20M", "HT", "1T", "128", "32",
	"FCC", "5G", "20M", "HT", "1T", "132", "32",
	"ETSI", "5G", "20M", "HT", "1T", "132", "32",
	"MKK", "5G", "20M", "HT", "1T", "132", "32",
	"FCC", "5G", "20M", "HT", "1T", "136", "32",
	"ETSI", "5G", "20M", "HT", "1T", "136", "32",
	"MKK", "5G", "20M", "HT", "1T", "136", "32",
	"FCC", "5G", "20M", "HT", "1T", "140", "26",
	"ETSI", "5G", "20M", "HT", "1T", "140", "32",
	"MKK", "5G", "20M", "HT", "1T", "140", "32",
	"FCC", "5G", "20M", "HT", "1T", "144", "26",
	"ETSI", "5G", "20M", "HT", "1T", "144", "63",
	"MKK", "5G", "20M", "HT", "1T", "144", "63",
	"FCC", "5G", "20M", "HT", "1T", "149", "32",
	"ETSI", "5G", "20M", "HT", "1T", "149", "63",
	"MKK", "5G", "20M", "HT", "1T", "149", "63",
	"FCC", "5G", "20M", "HT", "1T", "153", "32",
	"ETSI", "5G", "20M", "HT", "1T", "153", "63",
	"MKK", "5G", "20M", "HT", "1T", "153", "63",
	"FCC", "5G", "20M", "HT", "1T", "157", "32",
	"ETSI", "5G", "20M", "HT", "1T", "157", "63",
	"MKK", "5G", "20M", "HT", "1T", "157", "63",
	"FCC", "5G", "20M", "HT", "1T", "161", "32",
	"ETSI", "5G", "20M", "HT", "1T", "161", "63",
	"MKK", "5G", "20M", "HT", "1T", "161", "63",
	"FCC", "5G", "20M", "HT", "1T", "165", "32",
	"ETSI", "5G", "20M", "HT", "1T", "165", "63",
	"MKK", "5G", "20M", "HT", "1T", "165", "63",
	"FCC", "5G", "20M", "HT", "2T", "36", "28",
	"ETSI", "5G", "20M", "HT", "2T", "36", "20",
	"MKK", "5G", "20M", "HT", "2T", "36", "22",
	"FCC", "5G", "20M", "HT", "2T", "40", "30",
	"ETSI", "5G", "20M", "HT", "2T", "40", "20",
	"MKK", "5G", "20M", "HT", "2T", "40", "22",
	"FCC", "5G", "20M", "HT", "2T", "44", "30",
	"ETSI", "5G", "20M", "HT", "2T", "44", "20",
	"MKK", "5G", "20M", "HT", "2T", "44", "22",
	"FCC", "5G", "20M", "HT", "2T", "48", "30",
	"ETSI", "5G", "20M", "HT", "2T", "48", "20",
	"MKK", "5G", "20M", "HT", "2T", "48", "22",
	"FCC", "5G", "20M", "HT", "2T", "52", "30",
	"ETSI", "5G", "20M", "HT", "2T", "52", "20",
	"MKK", "5G", "20M", "HT", "2T", "52", "22",
	"FCC", "5G", "20M", "HT", "2T", "56", "30",
	"ETSI", "5G", "20M", "HT", "2T", "56", "20",
	"MKK", "5G", "20M", "HT", "2T", "56", "22",
	"FCC", "5G", "20M", "HT", "2T", "60", "30",
	"ETSI", "5G", "20M", "HT", "2T", "60", "20",
	"MKK", "5G", "20M", "HT", "2T", "60", "22",
	"FCC", "5G", "20M", "HT", "2T", "64", "28",
	"ETSI", "5G", "20M", "HT", "2T", "64", "20",
	"MKK", "5G", "20M", "HT", "2T", "64", "22",
	"FCC", "5G", "20M", "HT", "2T", "100", "26",
	"ETSI", "5G", "20M", "HT", "2T", "100", "20",
	"MKK", "5G", "20M", "HT", "2T", "100", "30",
	"FCC", "5G", "20M", "HT", "2T", "104", "30",
	"ETSI", "5G", "20M", "HT", "2T", "104", "20",
	"MKK", "5G", "20M", "HT", "2T", "104", "30",
	"FCC", "5G", "20M", "HT", "2T", "108", "32",
	"ETSI", "5G", "20M", "HT", "2T", "108", "20",
	"MKK", "5G", "20M", "HT", "2T", "108", "30",
	"FCC", "5G", "20M", "HT", "2T", "112", "32",
	"ETSI", "5G", "20M", "HT", "2T", "112", "20",
	"MKK", "5G", "20M", "HT", "2T", "112", "30",
	"FCC", "5G", "20M", "HT", "2T", "116", "32",
	"ETSI", "5G", "20M", "HT", "2T", "116", "20",
	"MKK", "5G", "20M", "HT", "2T", "116", "30",
	"FCC", "5G", "20M", "HT", "2T", "120", "32",
	"ETSI", "5G", "20M", "HT", "2T", "120", "20",
	"MKK", "5G", "20M", "HT", "2T", "120", "30",
	"FCC", "5G", "20M", "HT", "2T", "124", "32",
	"ETSI", "5G", "20M", "HT", "2T", "124", "20",
	"MKK", "5G", "20M", "HT", "2T", "124", "30",
	"FCC", "5G", "20M", "HT", "2T", "128", "32",
	"ETSI", "5G", "20M", "HT", "2T", "128", "20",
	"MKK", "5G", "20M", "HT", "2T", "128", "30",
	"FCC", "5G", "20M", "HT", "2T", "132", "32",
	"ETSI", "5G", "20M", "HT", "2T", "132", "20",
	"MKK", "5G", "20M", "HT", "2T", "132", "30",
	"FCC", "5G", "20M", "HT", "2T", "136", "30",
	"ETSI", "5G", "20M", "HT", "2T", "136", "20",
	"MKK", "5G", "20M", "HT", "2T", "136", "30",
	"FCC", "5G", "20M", "HT", "2T", "140", "26",
	"ETSI", "5G", "20M", "HT", "2T", "140", "20",
	"MKK", "5G", "20M", "HT", "2T", "140", "30",
	"FCC", "5G", "20M", "HT", "2T", "144", "26",
	"ETSI", "5G", "20M", "HT", "2T", "144", "63",
	"MKK", "5G", "20M", "HT", "2T", "144", "63",
	"FCC", "5G", "20M", "HT", "2T", "149", "32",
	"ETSI", "5G", "20M", "HT", "2T", "149", "63",
	"MKK", "5G", "20M", "HT", "2T", "149", "63",
	"FCC", "5G", "20M", "HT", "2T", "153", "32",
	"ETSI", "5G", "20M", "HT", "2T", "153", "63",
	"MKK", "5G", "20M", "HT", "2T", "153", "63",
	"FCC", "5G", "20M", "HT", "2T", "157", "32",
	"ETSI", "5G", "20M", "HT", "2T", "157", "63",
	"MKK", "5G", "20M", "HT", "2T", "157", "63",
	"FCC", "5G", "20M", "HT", "2T", "161", "32",
	"ETSI", "5G", "20M", "HT", "2T", "161", "63",
	"MKK", "5G", "20M", "HT", "2T", "161", "63",
	"FCC", "5G", "20M", "HT", "2T", "165", "32",
	"ETSI", "5G", "20M", "HT", "2T", "165", "63",
	"MKK", "5G", "20M", "HT", "2T", "165", "63",
	"FCC", "5G", "40M", "HT", "1T", "38", "22",
	"ETSI", "5G", "40M", "HT", "1T", "38", "30",
	"MKK", "5G", "40M", "HT", "1T", "38", "30",
	"FCC", "5G", "40M", "HT", "1T", "46", "30",
	"ETSI", "5G", "40M", "HT", "1T", "46", "30",
	"MKK", "5G", "40M", "HT", "1T", "46", "30",
	"FCC", "5G", "40M", "HT", "1T", "54", "30",
	"ETSI", "5G", "40M", "HT", "1T", "54", "30",
	"MKK", "5G", "40M", "HT", "1T", "54", "30",
	"FCC", "5G", "40M", "HT", "1T", "62", "24",
	"ETSI", "5G", "40M", "HT", "1T", "62", "30",
	"MKK", "5G", "40M", "HT", "1T", "62", "30",
	"FCC", "5G", "40M", "HT", "1T", "102", "24",
	"ETSI", "5G", "40M", "HT", "1T", "102", "30",
	"MKK", "5G", "40M", "HT", "1T", "102", "30",
	"FCC", "5G", "40M", "HT", "1T", "110", "30",
	"ETSI", "5G", "40M", "HT", "1T", "110", "30",
	"MKK", "5G", "40M", "HT", "1T", "110", "30",
	"FCC", "5G", "40M", "HT", "1T", "118", "30",
	"ETSI", "5G", "40M", "HT", "1T", "118", "30",
	"MKK", "5G", "40M", "HT", "1T", "118", "30",
	"FCC", "5G", "40M", "HT", "1T", "126", "30",
	"ETSI", "5G", "40M", "HT", "1T", "126", "30",
	"MKK", "5G", "40M", "HT", "1T", "126", "30",
	"FCC", "5G", "40M", "HT", "1T", "134", "30",
	"ETSI", "5G", "40M", "HT", "1T", "134", "30",
	"MKK", "5G", "40M", "HT", "1T", "134", "30",
	"FCC", "5G", "40M", "HT", "1T", "142", "30",
	"ETSI", "5G", "40M", "HT", "1T", "142", "63",
	"MKK", "5G", "40M", "HT", "1T", "142", "63",
	"FCC", "5G", "40M", "HT", "1T", "151", "30",
	"ETSI", "5G", "40M", "HT", "1T", "151", "63",
	"MKK", "5G", "40M", "HT", "1T", "151", "63",
	"FCC", "5G", "40M", "HT", "1T", "159", "30",
	"ETSI", "5G", "40M", "HT", "1T", "159", "63",
	"MKK", "5G", "40M", "HT", "1T", "159", "63",
	"FCC", "5G", "40M", "HT", "2T", "38", "20",
	"ETSI", "5G", "40M", "HT", "2T", "38", "20",
	"MKK", "5G", "40M", "HT", "2T", "38", "22",
	"FCC", "5G", "40M", "HT", "2T", "46", "30",
	"ETSI", "5G", "40M", "HT", "2T", "46", "20",
	"MKK", "5G", "40M", "HT", "2T", "46", "22",
	"FCC", "5G", "40M", "HT", "2T", "54", "30",
	"ETSI", "5G", "40M", "HT", "2T", "54", "20",
	"MKK", "5G", "40M", "HT", "2T", "54", "22",
	"FCC", "5G", "40M", "HT", "2T", "62", "22",
	"ETSI", "5G", "40M", "HT", "2T", "62", "20",
	"MKK", "5G", "40M", "HT", "2T", "62", "22",
	"FCC", "5G", "40M", "HT", "2T", "102", "22",
	"ETSI", "5G", "40M", "HT", "2T", "102", "20",
	"MKK", "5G", "40M", "HT", "2T", "102", "30",
	"FCC", "5G", "40M", "HT", "2T", "110", "30",
	"ETSI", "5G", "40M", "HT", "2T", "110", "20",
	"MKK", "5G", "40M", "HT", "2T", "110", "30",
	"FCC", "5G", "40M", "HT", "2T", "118", "30",
	"ETSI", "5G", "40M", "HT", "2T", "118", "20",
	"MKK", "5G", "40M", "HT", "2T", "118", "30",
	"FCC", "5G", "40M", "HT", "2T", "126", "30",
	"ETSI", "5G", "40M", "HT", "2T", "126", "20",
	"MKK", "5G", "40M", "HT", "2T", "126", "30",
	"FCC", "5G", "40M", "HT", "2T", "134", "30",
	"ETSI", "5G", "40M", "HT", "2T", "134", "20",
	"MKK", "5G", "40M", "HT", "2T", "134", "30",
	"FCC", "5G", "40M", "HT", "2T", "142", "30",
	"ETSI", "5G", "40M", "HT", "2T", "142", "63",
	"MKK", "5G", "40M", "HT", "2T", "142", "63",
	"FCC", "5G", "40M", "HT", "2T", "151", "30",
	"ETSI", "5G", "40M", "HT", "2T", "151", "63",
	"MKK", "5G", "40M", "HT", "2T", "151", "63",
	"FCC", "5G", "40M", "HT", "2T", "159", "30",
	"ETSI", "5G", "40M", "HT", "2T", "159", "63",
	"MKK", "5G", "40M", "HT", "2T", "159", "63",
	"FCC", "5G", "80M", "VHT", "1T", "42", "20",
	"ETSI", "5G", "80M", "VHT", "1T", "42", "30",
	"MKK", "5G", "80M", "VHT", "1T", "42", "28",
	"FCC", "5G", "80M", "VHT", "1T", "58", "20",
	"ETSI", "5G", "80M", "VHT", "1T", "58", "30",
	"MKK", "5G", "80M", "VHT", "1T", "58", "28",
	"FCC", "5G", "80M", "VHT", "1T", "106", "20",
	"ETSI", "5G", "80M", "VHT", "1T", "106", "30",
	"MKK", "5G", "80M", "VHT", "1T", "106", "30",
	"FCC", "5G", "80M", "VHT", "1T", "122", "30",
	"ETSI", "5G", "80M", "VHT", "1T", "122", "30",
	"MKK", "5G", "80M", "VHT", "1T", "122", "30",
	"FCC", "5G", "80M", "VHT", "1T", "138", "30",
	"ETSI", "5G", "80M", "VHT", "1T", "138", "63",
	"MKK", "5G", "80M", "VHT", "1T", "138", "63",
	"FCC", "5G", "80M", "VHT", "1T", "155", "30",
	"ETSI", "5G", "80M", "VHT", "1T", "155", "63",
	"MKK", "5G", "80M", "VHT", "1T", "155", "63",
	"FCC", "5G", "80M", "VHT", "2T", "42", "18",
	"ETSI", "5G", "80M", "VHT", "2T", "42", "20",
	"MKK", "5G", "80M", "VHT", "2T", "42", "22",
	"FCC", "5G", "80M", "VHT", "2T", "58", "18",
	"ETSI", "5G", "80M", "VHT", "2T", "58", "20",
	"MKK", "5G", "80M", "VHT", "2T", "58", "22",
	"FCC", "5G", "80M", "VHT", "2T", "106", "20",
	"ETSI", "5G", "80M", "VHT", "2T", "106", "20",
	"MKK", "5G", "80M", "VHT", "2T", "106", "30",
	"FCC", "5G", "80M", "VHT", "2T", "122", "30",
	"ETSI", "5G", "80M", "VHT", "2T", "122", "20",
	"MKK", "5G", "80M", "VHT", "2T", "122", "30",
	"FCC", "5G", "80M", "VHT", "2T", "138", "30",
	"ETSI", "5G", "80M", "VHT", "2T", "138", "63",
	"MKK", "5G", "80M", "VHT", "2T", "138", "63",
	"FCC", "5G", "80M", "VHT", "2T", "155", "30",
	"ETSI", "5G", "80M", "VHT", "2T", "155", "63",
	"MKK", "5G", "80M", "VHT", "2T", "155", "63"
};

void
odm_read_and_config_mp_8198f_txpwr_lmt(
	struct dm_struct	*dm
)
{
	u32	i = 0;
#if (DM_ODM_SUPPORT_TYPE == ODM_IOT)
	u32	array_len = sizeof(array_mp_8198f_txpwr_lmt)/sizeof(u8);
	u8	*array = (u8 *)array_mp_8198f_txpwr_lmt;
#else
	u32	array_len = sizeof(array_mp_8198f_txpwr_lmt)/sizeof(u8 *);
	u8	**array = (u8 **)array_mp_8198f_txpwr_lmt;
#endif

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	void	*adapter = dm->adapter;
	HAL_DATA_TYPE	*hal_data = GET_HAL_DATA(((PADAPTER)adapter));

	PlatformZeroMemory(hal_data->BufOfLinesPwrLmt, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
	hal_data->nLinesReadPwrLmt = array_len/7;
#endif

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_txpwr_lmt\n");

	for (i = 0; i < array_len; i += 7) {
#if (DM_ODM_SUPPORT_TYPE == ODM_IOT)
		u8	regulation = array[i];
		u8	band = array[i+1];
		u8	bandwidth = array[i+2];
		u8	rate = array[i+3];
		u8	rf_path = array[i+4];
		u8	chnl = array[i+5];
		u8	val = array[i+6];
#else
		u8	*regulation = array[i];
		u8	*band = array[i+1];
		u8	*bandwidth = array[i+2];
		u8	*rate = array[i+3];
		u8	*rf_path = array[i+4];
		u8	*chnl = array[i+5];
		u8	*val = array[i+6];
#endif

		odm_config_bb_txpwr_lmt_8198f(dm, regulation, band, bandwidth, rate, rf_path, chnl, val);
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
		rsprintf((char *)hal_data->BufOfLinesPwrLmt[i/7], 100, "\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\",",
		regulation, band, bandwidth, rate, rf_path, chnl, val);
#endif
	}

}

#endif /* end of HWIMG_SUPPORT*/

