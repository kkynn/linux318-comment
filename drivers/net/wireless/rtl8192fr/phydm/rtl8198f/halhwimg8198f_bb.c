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
*                           agc_tab.TXT
******************************************************************************/

u32 array_mp_8198f_agc_tab[] = {
		0x1D90, 0x300000FF,
		0x1D90, 0x300100FE,
		0x1D90, 0x300200FD,
		0x1D90, 0x300300FC,
		0x1D90, 0x300400FB,
		0x1D90, 0x300500FA,
		0x1D90, 0x300600F9,
		0x1D90, 0x300700F8,
		0x1D90, 0x300800F7,
		0x1D90, 0x300900F6,
		0x1D90, 0x300A00F5,
		0x1D90, 0x300B00F4,
		0x1D90, 0x300C00F3,
		0x1D90, 0x300D00F2,
		0x1D90, 0x300E00F1,
		0x1D90, 0x300F00F0,
		0x1D90, 0x301000EF,
		0x1D90, 0x301100EE,
		0x1D90, 0x301200ED,
		0x1D90, 0x301300EC,
		0x1D90, 0x301400EB,
		0x1D90, 0x301500EA,
		0x1D90, 0x301600E9,
		0x1D90, 0x301700E8,
		0x1D90, 0x301800E7,
		0x1D90, 0x301900E6,
		0x1D90, 0x301A00E5,
		0x1D90, 0x301B00E4,
		0x1D90, 0x301C00E3,
		0x1D90, 0x301D00E2,
		0x1D90, 0x301E00E1,
		0x1D90, 0x301F00E0,
		0x1D90, 0x302000C3,
		0x1D90, 0x302100C2,
		0x1D90, 0x302200C1,
		0x1D90, 0x302300A5,
		0x1D90, 0x302400A4,
		0x1D90, 0x302500A3,
		0x1D90, 0x302600A2,
		0x1D90, 0x302700A1,
		0x1D90, 0x30280068,
		0x1D90, 0x30290067,
		0x1D90, 0x302A0066,
		0x1D90, 0x302B0065,
		0x1D90, 0x302C0064,
		0x1D90, 0x302D0063,
		0x1D90, 0x302E0062,
		0x1D90, 0x302F0061,
		0x1D90, 0x30300060,
		0x1D90, 0x30310026,
		0x1D90, 0x30320025,
		0x1D90, 0x30330024,
		0x1D90, 0x30340023,
		0x1D90, 0x30350022,
		0x1D90, 0x30360021,
		0x1D90, 0x30370020,
		0x1D90, 0x30380001,
		0x1D90, 0x30390000,
		0x1D90, 0x303A0000,
		0x1D90, 0x303B0000,
		0x1D90, 0x303C0000,
		0x1D90, 0x303D0000,
		0x1D90, 0x303E0000,
		0x1D90, 0x303F0000,
		0x1D70, 0x00222222,
		0x1D70, 0x00202020,

};

void
odm_read_and_config_mp_8198f_agc_tab(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_agc_tab)/sizeof(u32);
	u32	*array = array_mp_8198f_agc_tab;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_agc_tab\n");

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
				odm_config_bb_agc_8198f(dm, v1, MASKDWORD, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_agc_tab(void)
{
		return 4;
}

/******************************************************************************
*                           phy_reg.TXT
******************************************************************************/

u32 array_mp_8198f_phy_reg[] = {
		0x1D0C, 0x00010000,
		0x800, 0x00000000,
		0x804, 0x56300000,
		0x808, 0x60156024,
		0x80C, 0x00000000,
		0x810, 0x10B019B0,
		0x814, 0x00004000,
		0x818, 0x030006F1,
		0x81C, 0x000302AA,
		0x820, 0x11111111,
		0x824, 0xCFC1CCC4,
		0x828, 0x3002186C,
		0x82C, 0x175D75DA,
		0x830, 0x17511459,
		0x834, 0x776995D7,
		0x838, 0x74777A7D,
		0x83C, 0xF9AA9982,
		0x840, 0x89AA9ABB,
		0x844, 0x0DEEDDC1,
		0x848, 0xCDEEDEFF,
		0x84C, 0x727D5555,
		0x850, 0x6F7A727D,
		0x854, 0x6C776F7A,
		0x858, 0x6F7A6C77,
		0x85C, 0x69746974,
		0x860, 0x6F7A6C77,
		0x864, 0x6C776C77,
		0x868, 0x727D6F7A,
		0x86C, 0x69D7B197,
		0x870, 0x2085A75E,
		0x874, 0x5582391D,
		0x878, 0x00C025BD,
		0x87C, 0x4140557D,
		0x880, 0x7A1D9D47,
		0x884, 0x1D67535F,
		0x888, 0x28572857,
		0x88C, 0x520E8A44,
		0x890, 0x89628C44,
		0x894, 0x72745F43,
		0x898, 0x03F02F0D,
		0x89C, 0x55B6886F,
		0x8A0, 0x07D0309F,
		0x8A4, 0x0040D28A,
		0x8A8, 0x00000044,
		0x8AC, 0x00000044,
		0x8B0, 0x00000043,
		0x8B4, 0x0000000D,
		0x8B8, 0x0000006F,
		0x8BC, 0x0000009F,
		0x8C0, 0x00000003,
		0x8C4, 0x00400000,
		0x8C8, 0x00000000,
		0x8CC, 0x00000000,
		0x8D0, 0x00000000,
		0x8D4, 0x00000000,
		0x8D8, 0x00000000,
		0x8DC, 0x00000000,
		0x8E0, 0x00000000,
		0x8E4, 0x00000000,
		0x8E8, 0x00000000,
		0x8EC, 0x00000000,
		0x8F0, 0x00000000,
		0x8F4, 0x00000000,
		0x8F8, 0x00000000,
		0x900, 0x00000000,
		0x904, 0x00000000,
		0x908, 0x000203CE,
		0x90C, 0x00000000,
		0x910, 0x00000000,
		0x914, 0x20000000,
		0x918, 0x20000000,
		0x91C, 0x20000000,
		0x920, 0x20000000,
		0x924, 0x00000000,
		0x928, 0x0000003A,
		0x92C, 0x0000003A,
		0x930, 0x0000003A,
		0x934, 0x0000003A,
		0x938, 0x0000000F,
		0x93C, 0x00000000,
		0x940, 0x4E1F3E81,
		0x944, 0x4E1F3E81,
		0x948, 0x4E1F3E81,
		0x94C, 0x4E1F3E81,
		0x950, 0x03020100,
		0x954, 0x07060504,
		0x958, 0x0B0A0908,
		0x95C, 0x0F0E0D0C,
		0x960, 0x13121110,
		0x964, 0x17161514,
		0x968, 0x03020100,
		0x96C, 0x07060504,
		0x970, 0x0B0A0908,
		0x974, 0x0F0E0D0C,
		0x978, 0x13121110,
		0x97C, 0x17161514,
		0x980, 0x03020100,
		0x984, 0x07060504,
		0x988, 0x0B0A0908,
		0x98C, 0x0F0E0D0C,
		0x990, 0x13121110,
		0x994, 0x17161514,
		0x998, 0x03020100,
		0x99C, 0x07060504,
		0x9A0, 0x0B0A0908,
		0x9A4, 0x0F0E0D0C,
		0x9A8, 0x13121110,
		0x9AC, 0x17161514,
		0x9B0, 0x00002200,
		0x9B4, 0x9246DB00,
		0x9B8, 0x00400064,
		0x9BC, 0x00000008,
		0x9C0, 0x00000001,
		0x9C4, 0x00000074,
		0x9C8, 0x00000064,
		0x9CC, 0x00000077,
		0x9D0, 0x00000064,
		0x9D4, 0x00000000,
		0x9D8, 0x00000000,
		0x9DC, 0x00000000,
		0x9E0, 0x00000018,
		0x9E4, 0x00000018,
		0x9E8, 0x00000018,
		0x9EC, 0x00000018,
		0x9F0, 0x000000FF,
		0x9F4, 0x00000000,
		0x9F8, 0x00000000,
		0xA00, 0x02001208,
		0xA04, 0x00000000,
		0xA08, 0x00000000,
		0xA0C, 0x00000000,
		0xA10, 0x00000000,
		0xA14, 0x00000000,
		0xA18, 0x00000000,
		0xA1C, 0x00000000,
		0xA20, 0x0B31B333,
		0xA24, 0x00275485,
		0xA28, 0x00166366,
		0xA2C, 0x00275485,
		0xA30, 0x00166366,
		0xA34, 0x00275485,
		0xA38, 0x00200400,
		0xA3C, 0x00200400,
		0xA40, 0xB359C5BD,
		0xA44, 0x3033BEBD,
		0xA48, 0x2A521254,
		0xA4C, 0xA2533345,
		0xA50, 0x605BE003,
		0xA54, 0x500089E8,
		0xA58, 0x00038000,
		0xA5C, 0x01000000,
		0xA60, 0x02000000,
		0xA64, 0x03000000,
		0xA68, 0x00000000,
		0xA6C, 0x00000000,
		0xA70, 0x00000000,
		0xA74, 0x00000000,
		0xA78, 0x00000000,
		0xA7C, 0x00000000,
		0xA80, 0x00000000,
		0xA84, 0x00000000,
		0xA88, 0x00000000,
		0xA8C, 0x00000000,
		0xA90, 0x00000000,
		0xA94, 0x00000000,
		0xA98, 0x00000000,
		0xA9C, 0x00000000,
		0xAA0, 0x00000000,
		0xAA4, 0x00000000,
		0xAA8, 0x00000000,
		0xAAC, 0x00000000,
		0xAB0, 0x00000000,
		0xAB4, 0x00000000,
		0xAB8, 0x00000000,
		0xABC, 0x00000000,
		0xAC0, 0x00000000,
		0xAC4, 0x00000000,
		0xAC8, 0x00000000,
		0xACC, 0x00000000,
		0xAD0, 0x00000000,
		0xAD4, 0x00000000,
		0xAD8, 0x00000000,
		0xADC, 0x00000000,
		0xAE0, 0x00000000,
		0xAE4, 0x00000000,
		0xAE8, 0x00000000,
		0xAEC, 0x00000000,
		0xAF0, 0x00000000,
		0xAF4, 0x00000000,
		0xAF8, 0x00000000,
		0xB00, 0x03900403,
		0xB04, 0x0AF99BF0,
		0xB08, 0x0000FE49,
		0xB0C, 0x00000000,
		0xB10, 0x00000000,
		0xB14, 0x00000000,
		0xB18, 0x00000000,
		0xB1C, 0x00000000,
		0xB20, 0x000013A7,
		0xB24, 0x00000000,
		0xB28, 0x00000000,
		0xB2C, 0x00000000,
		0xB30, 0x00000000,
		0xB34, 0x00000000,
		0xB38, 0x00000000,
		0xB3C, 0x00000000,
		0xB40, 0x00000000,
		0xB44, 0x00000000,
		0xB48, 0x00000000,
		0xB4C, 0x00000000,
		0xB50, 0x00000000,
		0xB54, 0x00000000,
		0xB58, 0x00000000,
		0xB5C, 0x00000000,
		0xB60, 0x00000000,
		0xB64, 0x00000000,
		0xB68, 0x00000000,
		0xB6C, 0x00000000,
		0xB70, 0x00000000,
		0xB74, 0x00000000,
		0xB78, 0x00000000,
		0xB7C, 0x00000000,
		0xB80, 0x00000000,
		0xB84, 0x00000000,
		0xB88, 0x00000000,
		0xB8C, 0x00000000,
		0xB90, 0x00000000,
		0xB94, 0x00000000,
		0xB98, 0x00000000,
		0xB9C, 0x00000000,
		0xBA0, 0x00000000,
		0xBA4, 0x00000000,
		0xBA8, 0x00000000,
		0xBAC, 0x00000000,
		0xBB0, 0x00000000,
		0xBB4, 0x00000000,
		0xBB8, 0x00000000,
		0xBBC, 0x00000000,
		0xBC0, 0x00000000,
		0xBC4, 0x00000000,
		0xBC8, 0x00000000,
		0xBCC, 0x00000000,
		0xBD0, 0x00000000,
		0xBD4, 0x00000000,
		0xBD8, 0x00000000,
		0xBDC, 0x00000000,
		0xBE0, 0x00000000,
		0xBE4, 0x00000000,
		0xBE8, 0x00000000,
		0xBEC, 0x00000000,
		0xBF0, 0x00000000,
		0xBF4, 0x00000000,
		0xBF8, 0x00000000,
		0xC00, 0x018BA0D6,
		0xC04, 0x00000001,
		0xC08, 0x00000000,
		0xC0C, 0x02F1E8B7,
		0xC10, 0x000000B0,
		0xC14, 0x0000D890,
		0xC18, 0x00087672,
		0xC1C, 0x15260000,
		0xC20, 0x00000000,
		0xC24, 0x40600000,
		0xC28, 0x06500F77,
		0xC2C, 0xE30020E1,
		0xC30, 0x140C9494,
		0xC34, 0x00A04946,
		0xC38, 0x011D4820,
		0xC3C, 0x168DB61B,
		0xC40, 0x009C50F8,
		0xC44, 0x2007BAD1,
		0xC48, 0xFFFFF7CC,
		0xC4C, 0xA000FFFF,
		0xC50, 0x20D0F800,
		0xC54, 0x941A0200,
		0xC58, 0x18380111,
		0xC5C, 0x006E01B8,
		0xC60, 0x2CA5555B,
		0xC64, 0x0A53005F,
		0xC68, 0x039A5300,
		0xC6C, 0x0265C2BA,
		0xC70, 0x000CEB21,
		0xC74, 0x0E149CA1,
		0xC78, 0x1AB4956B,
		0xC7C, 0x0000003F,
		0xC80, 0x00000000,
		0xC84, 0x00000000,
		0xC88, 0x000000BA,
		0xC8C, 0x00000021,
		0xC90, 0x000000A1,
		0xC94, 0x0000006B,
		0xC98, 0x00000000,
		0xC9C, 0x00000000,
		0xCA0, 0x00000000,
		0xCA4, 0x00000000,
		0xCA8, 0x00000000,
		0xCAC, 0x00000000,
		0xCB0, 0x00000000,
		0xCB4, 0x00000000,
		0xCB8, 0x00000000,
		0xCBC, 0x00000000,
		0xCC0, 0x00000000,
		0xCC4, 0x00000000,
		0xCC8, 0x00000000,
		0xCCC, 0x00000000,
		0xCD0, 0x00000000,
		0xCD4, 0x00000000,
		0xCD8, 0x00000000,
		0xCDC, 0x00000000,
		0xCE0, 0x00000000,
		0xCE4, 0x00000000,
		0xCE8, 0x00000000,
		0xCEC, 0x00000000,
		0xCF0, 0x00000000,
		0xCF4, 0x00000000,
		0xCF8, 0x00000000,
		0xD00, 0x1083A10A,
		0xD04, 0x0EC42948,
		0xD08, 0x10852108,
		0xD0C, 0x0CC41D08,
		0xD10, 0x108620EC,
		0xD14, 0x0CA42108,
		0xD18, 0x107620E8,
		0xD1C, 0x0E742108,
		0xD20, 0x0E8618C8,
		0xD24, 0x00000108,
		0xD28, 0x288C230C,
		0xD2C, 0x11C6320C,
		0xD30, 0x30CEBD98,
		0xD34, 0x10C31908,
		0xD38, 0x310A318C,
		0xD3C, 0x18C41D08,
		0xD40, 0x28CC4190,
		0xD44, 0x19062108,
		0xD48, 0x294A5A17,
		0xD4C, 0x00000108,
		0xD50, 0x10A3A908,
		0xD54, 0x10842148,
		0xD58, 0x14C5314A,
		0xD5C, 0x1086258C,
		0xD60, 0x10A42948,
		0xD64, 0x10842108,
		0xD68, 0x08C42108,
		0xD6C, 0x10842148,
		0xD70, 0x08822084,
		0xD74, 0x10841D04,
		0xD78, 0x08421088,
		0xD7C, 0x1083A104,
		0xD80, 0x10842108,
		0xD84, 0x1085294A,
		0xD88, 0x08822104,
		0xD8C, 0x10852948,
		0xD90, 0x08421084,
		0xD94, 0x10852104,
		0xD98, 0x08421084,
		0xD9C, 0x10863184,
		0xDA0, 0x1083B10A,
		0xDA4, 0x10842148,
		0xDA8, 0x1984718C,
		0xDAC, 0x108C33AF,
		0xDB0, 0x00000000,
		0xDB4, 0x00000000,
		0xDB8, 0x00000000,
		0xDBC, 0x00000000,
		0xDC0, 0x00000000,
		0xDC4, 0x00000000,
		0xDC8, 0x00000000,
		0xDCC, 0x00000000,
		0xDD0, 0x00000000,
		0xDD4, 0x00000000,
		0xDD8, 0x00000000,
		0xDDC, 0x00000000,
		0xDE0, 0x00000000,
		0xDE4, 0x00000000,
		0xDE8, 0x00000000,
		0xDEC, 0x00000000,
		0xDF0, 0x00000000,
		0xDF4, 0x00000000,
		0xDF8, 0x00000000,
		0x1800, 0x00033312,
		0x1804, 0x00033312,
		0x180C, 0x10000220,
		0x1810, 0xD8001402,
		0x1814, 0xC0000120,
		0x1818, 0x00000280,
		0x181C, 0x00000000,
		0x1820, 0xD8001402,
		0x1824, 0xC0000120,
		0x1828, 0x00000280,
		0x182C, 0x00000000,
		0x1834, 0x00000000,
		0x1838, 0x00000000,
		0x183C, 0x00000000,
		0x1840, 0x00000000,
		0x1844, 0x00000000,
		0x1848, 0x00000000,
		0x184C, 0x00000000,
		0x1850, 0x00000000,
		0x1854, 0x00000000,
		0x1858, 0x00000000,
		0x185C, 0x00000000,
		0x1860, 0xF0040000,
		0x1864, 0x00000000,
		0x1868, 0x00000000,
		0x186C, 0x00000000,
		0x1870, 0x00000000,
		0x1874, 0x00000000,
		0x1878, 0x00000000,
		0x187C, 0x00000000,
		0x1880, 0x00000000,
		0x1884, 0x00000000,
		0x1888, 0x00000000,
		0x188C, 0x00000000,
		0x1890, 0x00000000,
		0x1894, 0x00000000,
		0x1898, 0x00000000,
		0x189C, 0x00000000,
		0x18A0, 0x00510000,
		0x18A4, 0x083C1F7F,
		0x18A8, 0x00004998,
		0x18AC, 0x00000000,
		0x18B0, 0x03000F09,
		0x18B4, 0x00000000,
		0x18B8, 0x00000000,
		0x18BC, 0xC100FFC0,
		0x18C0, 0x000001FF,
		0x18C4, 0x00000004,
		0x18C8, 0x00007FE0,
		0x18CC, 0x03000F09,
		0x18D0, 0x00000000,
		0x18D4, 0x00000000,
		0x18D8, 0xC100FFC0,
		0x18DC, 0x000001FF,
		0x18E0, 0x00000004,
		0x18E4, 0x00007FE0,
		0x18E8, 0x00013E00,
		0x18EC, 0x1E008000,
		0x18F0, 0x00000000,
		0x18F4, 0x00000000,
		0x18F8, 0x00000000,
		0x1900, 0x85857474,
		0x1904, 0xB7B7C8C8,
		0x1908, 0x00777777,
		0x190C, 0x77776666,
		0x1910, 0x00033333,
		0x1914, 0xAAAC875A,
		0x1918, 0x2AA2A861,
		0x191C, 0x2AAAA861,
		0x1920, 0x00878766,
		0x1924, 0x004E4924,
		0x1928, 0x566DB6C0,
		0x192C, 0x00407090,
		0x1930, 0xB85C0924,
		0x1934, 0x00B4A298,
		0x1938, 0x00030151,
		0x193C, 0x0058C618,
		0x1940, 0x41000000,
		0x1944, 0x00000BCB,
		0x1948, 0xAAAAAAAA,
		0x194C, 0x00000000,
		0x1950, 0x00000000,
		0x1954, 0x00000000,
		0x1958, 0x00000000,
		0x195C, 0x00000000,
		0x1960, 0x00000000,
		0x1964, 0x00000000,
		0x1968, 0x00000000,
		0x196C, 0x00000000,
		0x1970, 0x00000000,
		0x1974, 0x00000000,
		0x1978, 0x00000000,
		0x197C, 0x00000000,
		0x1980, 0x00000000,
		0x1984, 0x00000000,
		0x1988, 0x00000000,
		0x198C, 0x00000000,
		0x1990, 0x00000000,
		0x1994, 0x00000000,
		0x1998, 0x00000000,
		0x199C, 0x00000000,
		0x19A0, 0x00000000,
		0x19A4, 0x00000000,
		0x19A8, 0x00000000,
		0x19AC, 0x00000000,
		0x19B0, 0x00000000,
		0x19B4, 0x00000000,
		0x19B8, 0x00000000,
		0x19BC, 0x00000000,
		0x19C0, 0x00000000,
		0x19C4, 0x00000000,
		0x19C8, 0x00000000,
		0x19CC, 0x00000000,
		0x19D0, 0x00000000,
		0x19D4, 0x00000000,
		0x19D8, 0x00000000,
		0x19DC, 0x00000000,
		0x19E0, 0x00000000,
		0x19E4, 0x00000000,
		0x19E8, 0x00000000,
		0x19EC, 0x00000000,
		0x19F0, 0x00000000,
		0x19F4, 0x00000000,
		0x19F8, 0x00000000,
		0x1A00, 0x00D047C8,
		0x1A04, 0xC0FF0008,
		0x1A08, 0x88838300,
		0x1A0C, 0x2E20100F,
		0x1A10, 0x9500BB78,
		0x1A14, 0x1114D028,
		0x1A18, 0x00881117,
		0x1A1C, 0x89140F00,
		0x1A20, 0xE82C0000,
		0x1A24, 0x64B80C1C,
		0x1A28, 0x00158810,
		0x1A2C, 0x10998020,
		0x1A70, 0x00008000,
		0x1A74, 0x80800148,
		0x1A78, 0x000089F0,
		0x1A7C, 0x225B0606,
		0x1A80, 0x208A3210,
		0x1A84, 0x80200200,
		0x1A88, 0x00000000,
		0x1A8C, 0x00000000,
		0x1A90, 0x00000000,
		0x1A94, 0x00000000,
		0x1A98, 0x00000000,
		0x1A9C, 0x00460000,
		0x1AA0, 0x00000000,
		0x1AA4, 0x00020014,
		0x1AA8, 0xBA0A0000,
		0x1AAC, 0x01235667,
		0x1AB0, 0x00000000,
		0x1AB4, 0x00201402,
		0x1AB8, 0x0000001C,
		0x1ABC, 0x0200F7FF,
		0x1AC0, 0x54C0A742,
		0x1AC4, 0x00000000,
		0x1AC8, 0x00000808,
		0x1ACC, 0x00000707,
		0x1AD0, 0x9D0529D0,
		0x1AD4, 0x0D9D8452,
		0x1AD8, 0x9E024024,
		0x1ADC, 0x0039C003,
		0x1AE0, 0x00600391,
		0x1AE4, 0x08000080,
		0x1C00, 0x00000000,
		0x1C04, 0x00000000,
		0x1C08, 0x00000000,
		0x1C0C, 0x00000000,
		0x1C10, 0x00000000,
		0x1C14, 0x00000000,
		0x1C18, 0x00000000,
		0x1C1C, 0x00000000,
		0x1C20, 0x00003F20,
		0x1C24, 0x00010002,
		0x1C28, 0x00000000,
		0x1C2C, 0x45300000,
		0x1C30, 0x00000000,
		0x1C34, 0xE4E40000,
		0x1C38, 0xFFA1005E,
		0x1C3C, 0x00000000,
		0x1C40, 0x8F58AA77,
		0x1C44, 0x04400300,
		0x1C48, 0x00000000,
		0x1C4C, 0x00000200,
		0x1C50, 0x8F58AA77,
		0x1C54, 0x7DFFFFFF,
		0x1C58, 0x00000000,
		0x1C5C, 0x00000000,
		0x1C60, 0x0F030032,
		0x1C64, 0x00000000,
		0x1C68, 0x007F0000,
		0x1C6C, 0x00000000,
		0x1C70, 0x00000000,
		0x1C74, 0x00000000,
		0x1C78, 0x00020000,
		0x1C7C, 0x00300000,
		0x1C80, 0x0F3CF000,
		0x1C84, 0x03D140CE,
		0x1C88, 0xC8404483,
		0x1C8C, 0x08305A20,
		0x1C90, 0x00E41700,
		0x1C94, 0x00000000,
		0x1C98, 0x00000000,
		0x1C9C, 0x00000000,
		0x1CA0, 0x00000000,
		0x1CA4, 0x00000000,
		0x1CA8, 0x00000000,
		0x1CAC, 0xE424A2CC,
		0x1CB0, 0x00000000,
		0x1CB4, 0x00000000,
		0x1CB8, 0x24800000,
		0x1CBC, 0x60004800,
		0x1CC0, 0x24800000,
		0x1CC4, 0x60004800,
		0x1CC8, 0x00000000,
		0x1CCC, 0x00000000,
		0x1CD0, 0x00000000,
		0x1CD4, 0x02024B00,
		0x1CD8, 0x04000000,
		0x1CDC, 0x10000000,
		0x1CE0, 0x00000000,
		0x1CE4, 0x00000000,
		0x1CE8, 0x00000000,
		0x1CEC, 0x00000000,
		0x1CF0, 0x00000000,
		0x1CF4, 0x00000000,
		0x1CF8, 0x00000000,
		0x1D00, 0x00000000,
		0x1D04, 0x00000000,
		0x1D08, 0xA0000000,
		0x1D10, 0x0000BBBB,
		0x1D14, 0x77777777,
		0x1D18, 0x99999999,
		0x1D1C, 0x99999999,
		0x1D20, 0x00000000,
		0x1D24, 0x00000000,
		0x1D28, 0x00000000,
		0x1D2C, 0x40000000,
		0x1D30, 0x50649F00,
		0x1D34, 0x00000000,
		0x1D38, 0x00000000,
		0x1D3C, 0xF8000000,
		0x1D40, 0x00000000,
		0x1D44, 0x03000000,
		0x1D48, 0x03000044,
		0x1D4C, 0x00000000,
		0x1D50, 0x00000000,
		0x1D54, 0x00000000,
		0x1D58, 0x80800000,
		0x1D5C, 0x00000000,
		0x1D60, 0x00000000,
		0x1D64, 0x88000000,
		0x1D68, 0x00000000,
		0x1D6C, 0x666D8000,
		0x1D70, 0x20202020,
		0x1D74, 0x20202020,
		0x1D78, 0x18189818,
		0x1D7C, 0x0005A000,
		0x1D80, 0x00080000,
		0x1D84, 0x00080000,
		0x1D88, 0x000000EF,
		0x1D8C, 0x0C0C0C0C,
		0x1D90, 0x103F003F,
		0x1D94, 0x00000000,
		0x1D98, 0x00000000,
		0x1D9C, 0x00000000,
		0x1DA0, 0x00000000,
		0x1DA4, 0x00000000,
		0x1DA8, 0x00000000,
		0x1DAC, 0x00000000,
		0x1DB0, 0x00000000,
		0x1DB4, 0x00000000,
		0x1DB8, 0x00000000,
		0x1DBC, 0x00000000,
		0x1DC0, 0x00000000,
		0x1DC4, 0x00000000,
		0x1DC8, 0x00000000,
		0x1DCC, 0x00000000,
		0x1DD0, 0x00000000,
		0x1DD4, 0x00000000,
		0x1DD8, 0x00000000,
		0x1DDC, 0x0F9F0000,
		0x1DE0, 0x01010000,
		0x1DE4, 0x05210123,
		0x1DE8, 0x7FFF4040,
		0x1DEC, 0x00000000,
		0x1DF0, 0x00000000,
		0x1DF4, 0x00000000,
		0x1DF8, 0x00000000,
		0x1E00, 0x00000000,
		0x1E04, 0x00000000,
		0x1E08, 0x00000000,
		0x1E0C, 0x00000000,
		0x1E10, 0x00000000,
		0x1E14, 0x00000000,
		0x1E18, 0x00000000,
		0x1E1C, 0x00000000,
		0x1E20, 0x00000000,
		0x1E24, 0x80003000,
		0x1E28, 0x000CCCC3,
		0x1E2C, 0xE4E4E4E4,
		0x1E30, 0xE4E4E4E4,
		0x1E34, 0xF30011AC,
		0x1E38, 0x00000000,
		0x1E3C, 0x00000000,
		0x1E40, 0x00000000,
		0x1E44, 0x00000000,
		0x1E48, 0x00000000,
		0x1E4C, 0x00000000,
		0x1E50, 0x00000000,
		0x1E54, 0x00000000,
		0x1E58, 0x00000000,
		0x1E5C, 0xC0000000,
		0x1E60, 0x00000000,
		0x1E64, 0xF3A00001,
		0x1E68, 0x0028846E,
		0x1E6C, 0x402F4906,
		0x1E70, 0x00000000,
		0x1E74, 0x00000000,
		0x1E78, 0x00000000,
		0x1E7C, 0x00000000,
		0x1E80, 0x00000000,
		0x1E84, 0x00000000,
		0x1E88, 0x0000FC1C,
		0x1E8C, 0x00000000,
		0x1E90, 0x00000000,
		0x1E94, 0x00000000,
		0x1E98, 0x00000000,
		0x1E9C, 0x00000000,
		0x1EA0, 0x00000000,
		0x1EA4, 0x00000000,
		0x1EA8, 0xAA464646,
		0x1EAC, 0x01800030,
		0x1EB0, 0x00003000,
		0x1EB4, 0x31000002,
		0x1EB8, 0x00000000,
		0x1EBC, 0x00000000,
		0x1EC0, 0x00000000,
		0x1EC4, 0x00000000,
		0x1EC8, 0x00000000,
		0x1ECC, 0x00000000,
		0x1ED0, 0x00000000,
		0x1ED4, 0x00000000,
		0x1ED8, 0x00000000,
		0x1EDC, 0x00000000,
		0x1EE0, 0x00000000,
		0x1EE4, 0x00000000,
		0x1EE8, 0x00000000,
		0x1EEC, 0x00000000,
		0x1EF0, 0x00000000,
		0x1EF4, 0x00000000,
		0x1EF8, 0x00000000,
		0x3A00, 0x00000000,
		0x3A04, 0x00000000,
		0x3A08, 0x00000000,
		0x3A0C, 0x00000000,
		0x3A10, 0x00000000,
		0x3A14, 0x00000000,
		0x3A18, 0x00000000,
		0x3A1C, 0x00000000,
		0x3A20, 0x00000000,
		0x3A24, 0x00000000,
		0x3A28, 0x00000000,
		0x3A2C, 0x00000000,
		0x3A30, 0x00000000,
		0x3A34, 0x00000000,
		0x3A38, 0x00000000,
		0x3A3C, 0x00000000,
		0x3A40, 0x00000000,
		0x3A44, 0x00000000,
		0x3A48, 0x00000000,
		0x3A4C, 0x00000000,
		0x3A50, 0x00000000,
		0x4000, 0x85857575,
		0x4004, 0xB7B7B7B7,
		0x4008, 0x00777777,
		0x400C, 0x77776666,
		0x4010, 0x00033333,
		0x4014, 0xAAAC875A,
		0x4018, 0x2AA2A8A2,
		0x401C, 0x2AAAA8A2,
		0x4020, 0x00878766,
		0x4024, 0x004E4924,
		0x4028, 0x566DB6C0,
		0x402C, 0x00407090,
		0x4030, 0xB85C0924,
		0x4034, 0x00B4A298,
		0x4038, 0x00030151,
		0x403C, 0x0058C618,
		0x4040, 0x41000000,
		0x4044, 0x00000BCB,
		0x4048, 0xAAAAAAAA,
		0x404C, 0x00000000,
		0x4050, 0x00000000,
		0x4054, 0x00000000,
		0x4058, 0x00000000,
		0x405C, 0x00000000,
		0x4060, 0x00000000,
		0x4064, 0x00000000,
		0x4068, 0x00000000,
		0x406C, 0x00000000,
		0x4070, 0x00000000,
		0x4074, 0x00000000,
		0x4078, 0x00000000,
		0x407C, 0x00000000,
		0x4080, 0x00000000,
		0x4084, 0x00000000,
		0x4088, 0x00000000,
		0x408C, 0x00000000,
		0x4090, 0x00000000,
		0x4094, 0x00000000,
		0x4098, 0x00000000,
		0x409C, 0x00000000,
		0x40A0, 0x00000000,
		0x40A4, 0x00000000,
		0x40A8, 0x00000000,
		0x40AC, 0x00000000,
		0x40B0, 0x00000000,
		0x40B4, 0x00000000,
		0x40B8, 0x00000000,
		0x40BC, 0x00000000,
		0x40C0, 0x00000000,
		0x40C4, 0x00000000,
		0x40C8, 0x00000000,
		0x40CC, 0x00000000,
		0x40D0, 0x00000000,
		0x40D4, 0x00000000,
		0x40D8, 0x00000000,
		0x40DC, 0x00000000,
		0x40E0, 0x00000000,
		0x40E4, 0x00000000,
		0x40E8, 0x00000000,
		0x40EC, 0x00000000,
		0x40F0, 0x00000000,
		0x40F4, 0x00000000,
		0x40F8, 0x00000000,
		0x4100, 0x00033312,
		0x4104, 0x00033312,
		0x410C, 0x10000220,
		0x4110, 0xD8001402,
		0x4114, 0xC0000120,
		0x4118, 0x00000280,
		0x411C, 0x00000000,
		0x4120, 0xD8001402,
		0x4124, 0xC0000120,
		0x4128, 0x00000280,
		0x412C, 0x00000000,
		0x4134, 0x00000000,
		0x4138, 0x00000000,
		0x413C, 0x00000000,
		0x4140, 0x00000000,
		0x4144, 0x00000000,
		0x4148, 0x00000000,
		0x414C, 0x00000000,
		0x4150, 0x00000000,
		0x4154, 0x00000000,
		0x4158, 0x00000000,
		0x415C, 0x00000000,
		0x4160, 0xF0040000,
		0x4164, 0x00000000,
		0x4168, 0x00000000,
		0x416C, 0x00000000,
		0x4170, 0x00000000,
		0x4174, 0x00000000,
		0x4178, 0x00000000,
		0x417C, 0x00000000,
		0x4180, 0x00000000,
		0x4184, 0x00000000,
		0x4188, 0x00000000,
		0x418C, 0x00000000,
		0x4190, 0x00000000,
		0x4194, 0x00000000,
		0x4198, 0x00000000,
		0x419C, 0x00000000,
		0x41A0, 0x00510000,
		0x41A4, 0x083C1F7F,
		0x41A8, 0x00004998,
		0x41AC, 0x00000000,
		0x41B0, 0x03000F09,
		0x41B4, 0x00000000,
		0x41B8, 0x00000000,
		0x41BC, 0xC100FFC0,
		0x41C0, 0x000001FF,
		0x41C4, 0x00000004,
		0x41C8, 0x00007FE0,
		0x41CC, 0x03000F09,
		0x41D0, 0x00000000,
		0x41D4, 0x00000000,
		0x41D8, 0xC100FFC0,
		0x41DC, 0x000001FF,
		0x41E0, 0x00000004,
		0x41E4, 0x00007FE0,
		0x41E8, 0x00013E00,
		0x41EC, 0x1E008000,
		0x41F0, 0x00000000,
		0x41F4, 0x00000000,
		0x41F8, 0x00000000,
		0x5000, 0x63636363,
		0x5004, 0xB7B7B7B7,
		0x5008, 0x00888888,
		0x500C, 0x88887777,
		0x5010, 0x00033333,
		0x5014, 0xAEBC875B,
		0x5018, 0x2BA6B924,
		0x501C, 0x2BAAB924,
		0x5020, 0x00878766,
		0x5024, 0x004E4924,
		0x5028, 0x566DB6C0,
		0x502C, 0x00407090,
		0x5030, 0xB85C0924,
		0x5034, 0x00B4A298,
		0x5038, 0x00030151,
		0x503C, 0x0058C618,
		0x5040, 0x41000000,
		0x5044, 0x00000BCB,
		0x5048, 0xAAAAAAAA,
		0x504C, 0x00000000,
		0x5050, 0x00000000,
		0x5054, 0x00000000,
		0x5058, 0x00000000,
		0x505C, 0x00000000,
		0x5060, 0x00000000,
		0x5064, 0x00000000,
		0x5068, 0x00000000,
		0x506C, 0x00000000,
		0x5070, 0x00000000,
		0x5074, 0x00000000,
		0x5078, 0x00000000,
		0x507C, 0x00000000,
		0x5080, 0x00000000,
		0x5084, 0x00000000,
		0x5088, 0x00000000,
		0x508C, 0x00000000,
		0x5090, 0x00000000,
		0x5094, 0x00000000,
		0x5098, 0x00000000,
		0x509C, 0x00000000,
		0x50A0, 0x00000000,
		0x50A4, 0x00000000,
		0x50A8, 0x00000000,
		0x50AC, 0x00000000,
		0x50B0, 0x00000000,
		0x50B4, 0x00000000,
		0x50B8, 0x00000000,
		0x50BC, 0x00000000,
		0x50C0, 0x00000000,
		0x50C4, 0x00000000,
		0x50C8, 0x00000000,
		0x50CC, 0x00000000,
		0x50D0, 0x00000000,
		0x50D4, 0x00000000,
		0x50D8, 0x00000000,
		0x50DC, 0x00000000,
		0x50E0, 0x00000000,
		0x50E4, 0x00000000,
		0x50E8, 0x00000000,
		0x50EC, 0x00000000,
		0x50F0, 0x00000000,
		0x50F4, 0x00000000,
		0x50F8, 0x00000000,
		0x5100, 0x42424242,
		0x5104, 0xB7B76363,
		0x5108, 0x77778788,
		0x510C, 0x88887777,
		0x5110, 0x00033333,
		0x5114, 0xAEBC865B,
		0x5118, 0x2BA6B924,
		0x511C, 0x2BAAB924,
		0x5120, 0x00868655,
		0x5124, 0x004E4924,
		0x5128, 0x566DB6C0,
		0x512C, 0x00407090,
		0x5130, 0xB85C0924,
		0x5134, 0x00B4A298,
		0x5138, 0x00030151,
		0x513C, 0x0058C618,
		0x5140, 0x41000000,
		0x5144, 0x00000BCB,
		0x5148, 0xAAAAAAAA,
		0x514C, 0x00000000,
		0x5150, 0x00000000,
		0x5154, 0x00000000,
		0x5158, 0x00000000,
		0x515C, 0x00000000,
		0x5160, 0x00000000,
		0x5164, 0x00000000,
		0x5168, 0x00000000,
		0x516C, 0x00000000,
		0x5170, 0x00000000,
		0x5174, 0x00000000,
		0x5178, 0x00000000,
		0x517C, 0x00000000,
		0x5180, 0x00000000,
		0x5184, 0x00000000,
		0x5188, 0x00000000,
		0x518C, 0x00000000,
		0x5190, 0x00000000,
		0x5194, 0x00000000,
		0x5198, 0x00000000,
		0x519C, 0x00000000,
		0x51A0, 0x00000000,
		0x51A4, 0x00000000,
		0x51A8, 0x00000000,
		0x51AC, 0x00000000,
		0x51B0, 0x00000000,
		0x51B4, 0x00000000,
		0x51B8, 0x00000000,
		0x51BC, 0x00000000,
		0x51C0, 0x00000000,
		0x51C4, 0x00000000,
		0x51C8, 0x00000000,
		0x51CC, 0x00000000,
		0x51D0, 0x00000000,
		0x51D4, 0x00000000,
		0x51D8, 0x00000000,
		0x51DC, 0x00000000,
		0x51E0, 0x00000000,
		0x51E4, 0x00000000,
		0x51E8, 0x00000000,
		0x51EC, 0x00000000,
		0x51F0, 0x00000000,
		0x51F4, 0x00000000,
		0x51F8, 0x00000000,
		0x5200, 0x00033312,
		0x5204, 0x00033312,
		0x520C, 0x10000220,
		0x5210, 0xD8001402,
		0x5214, 0xC0000120,
		0x5218, 0x00000280,
		0x521C, 0x00000000,
		0x5220, 0xD8001402,
		0x5224, 0xC0000120,
		0x5228, 0x00000280,
		0x522C, 0x00000000,
		0x5234, 0x00000000,
		0x5238, 0x00000000,
		0x523C, 0x00000000,
		0x5240, 0x00000000,
		0x5244, 0x00000000,
		0x5248, 0x00000000,
		0x524C, 0x00000000,
		0x5250, 0x00000000,
		0x5254, 0x00000000,
		0x5258, 0x00000000,
		0x525C, 0x00000000,
		0x5260, 0xF0040000,
		0x5264, 0x00000000,
		0x5268, 0x00000000,
		0x526C, 0x00000000,
		0x5270, 0x00000000,
		0x5274, 0x00000000,
		0x5278, 0x00000000,
		0x527C, 0x00000000,
		0x5280, 0x00000000,
		0x5284, 0x00000000,
		0x5288, 0x00000000,
		0x528C, 0x00000000,
		0x5290, 0x00000000,
		0x5294, 0x00000000,
		0x5298, 0x00000000,
		0x529C, 0x00000000,
		0x52A0, 0x00510000,
		0x52A4, 0x083C1F7F,
		0x52A8, 0x00004998,
		0x52AC, 0x00000000,
		0x52B0, 0x03000F09,
		0x52B4, 0x00000000,
		0x52B8, 0x00000000,
		0x52BC, 0xC100FFC0,
		0x52C0, 0x000001FF,
		0x52C4, 0x00000004,
		0x52C8, 0x00007FE0,
		0x52CC, 0x03000F09,
		0x52D0, 0x00000000,
		0x52D4, 0x00000000,
		0x52D8, 0xC100FFC0,
		0x52DC, 0x000001FF,
		0x52E0, 0x00000004,
		0x52E4, 0x00007FE0,
		0x52E8, 0x00013E00,
		0x52EC, 0x1E008000,
		0x52F0, 0x00000000,
		0x52F4, 0x00000000,
		0x52F8, 0x00000000,
		0x5300, 0x00033312,
		0x5304, 0x00033312,
		0x530C, 0x10000220,
		0x5310, 0xD8001402,
		0x5314, 0xC0000120,
		0x5318, 0x00000280,
		0x531C, 0x00000000,
		0x5320, 0xD8001402,
		0x5324, 0xC0000120,
		0x5328, 0x00000280,
		0x532C, 0x00000000,
		0x5334, 0x00000000,
		0x5338, 0x00000000,
		0x533C, 0x00000000,
		0x5340, 0x00000000,
		0x5344, 0x00000000,
		0x5348, 0x00000000,
		0x534C, 0x00000000,
		0x5350, 0x00000000,
		0x5354, 0x00000000,
		0x5358, 0x00000000,
		0x535C, 0x00000000,
		0x5360, 0xF0040000,
		0x5364, 0x00000000,
		0x5368, 0x00000000,
		0x536C, 0x00000000,
		0x5370, 0x00000000,
		0x5374, 0x00000000,
		0x5378, 0x00000000,
		0x537C, 0x00000000,
		0x5380, 0x00000000,
		0x5384, 0x00000000,
		0x5388, 0x00000000,
		0x538C, 0x00000000,
		0x5390, 0x00000000,
		0x5394, 0x00000000,
		0x5398, 0x00000000,
		0x539C, 0x00000000,
		0x53A0, 0x00510000,
		0x53A4, 0x083C1F7F,
		0x53A8, 0x00004998,
		0x53AC, 0x00000000,
		0x53B0, 0x03000F09,
		0x53B4, 0x00000000,
		0x53B8, 0x00000000,
		0x53BC, 0xC100FFC0,
		0x53C0, 0x000001FF,
		0x53C4, 0x00000004,
		0x53C8, 0x00007FE0,
		0x53CC, 0x03000F09,
		0x53D0, 0x00000000,
		0x53D4, 0x00000000,
		0x53D8, 0xC100FFC0,
		0x53DC, 0x000001FF,
		0x53E0, 0x00000004,
		0x53E4, 0x00007FE0,
		0x53E8, 0x00013E00,
		0x53EC, 0x1E008000,
		0x53F0, 0x00000000,
		0x53F4, 0x00000000,
		0x53F8, 0x00000000,
		0x1830, 0x700B8041,
		0x1830, 0x700B8041,
		0x1830, 0x70144041,
		0x1830, 0x70244041,
		0x1830, 0x70344041,
		0x1830, 0x70444041,
		0x1830, 0x705B8041,
		0x1830, 0x70644041,
		0x1830, 0x707B8041,
		0x1830, 0x708B8041,
		0x1830, 0x709B8041,
		0x1830, 0x70AB8041,
		0x1830, 0x70BB8041,
		0x1830, 0x70CB8041,
		0x1830, 0x70DB8041,
		0x1830, 0x70EB8041,
		0x1830, 0x70FB8041,
		0x1830, 0x70FB8041,
		0x4130, 0x700B8041,
		0x4130, 0x700B8041,
		0x4130, 0x70144041,
		0x4130, 0x70244041,
		0x4130, 0x70344041,
		0x4130, 0x70444041,
		0x4130, 0x705B8041,
		0x4130, 0x70644041,
		0x4130, 0x707B8041,
		0x4130, 0x708B8041,
		0x4130, 0x709B8041,
		0x4130, 0x70AB8041,
		0x4130, 0x70BB8041,
		0x4130, 0x70CB8041,
		0x4130, 0x70DB8041,
		0x4130, 0x70EB8041,
		0x4130, 0x70FB8041,
		0x4130, 0x70FB8041,
		0x5230, 0x700B8041,
		0x5230, 0x700B8041,
		0x5230, 0x70144041,
		0x5230, 0x70244041,
		0x5230, 0x70344041,
		0x5230, 0x70444041,
		0x5230, 0x705B8041,
		0x5230, 0x70644041,
		0x5230, 0x707B8041,
		0x5230, 0x708B8041,
		0x5230, 0x709B8041,
		0x5230, 0x70AB8041,
		0x5230, 0x70BB8041,
		0x5230, 0x70CB8041,
		0x5230, 0x70DB8041,
		0x5230, 0x70EB8041,
		0x5230, 0x70FB8041,
		0x5230, 0x70FB8041,
		0x5330, 0x700B8041,
		0x5330, 0x700B8041,
		0x5330, 0x70144041,
		0x5330, 0x70244041,
		0x5330, 0x70344041,
		0x5330, 0x70444041,
		0x5330, 0x705B8041,
		0x5330, 0x70644041,
		0x5330, 0x707B8041,
		0x5330, 0x708B8041,
		0x5330, 0x709B8041,
		0x5330, 0x70AB8041,
		0x5330, 0x70BB8041,
		0x5330, 0x70CB8041,
		0x5330, 0x70DB8041,
		0x5330, 0x70EB8041,
		0x5330, 0x70FB8041,
		0x5330, 0x70FB8041,
		0x1D0C, 0x00000000,
		0x1D0C, 0x00010000,

};

void
odm_read_and_config_mp_8198f_phy_reg(
	struct	dm_struct *dm
)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8198f_phy_reg)/sizeof(u32);
	u32	*array = array_mp_8198f_phy_reg;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_phy_reg\n");

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
				odm_config_bb_phy_8198f(dm, v1, MASKDWORD, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8198f_phy_reg(void)
{
		return 4;
}

/******************************************************************************
*                           phy_reg_pg.TXT
******************************************************************************/

u32 array_mp_8198f_phy_reg_pg[] = {
	0, 0, 0, 0x00000c20, 0xffffffff, 0x32343638,
	0, 0, 0, 0x00000c24, 0xffffffff, 0x36384042,
	0, 0, 0, 0x00000c28, 0xffffffff, 0x28303234,
	0, 0, 0, 0x00000c2c, 0xffffffff, 0x34363840,
	0, 0, 0, 0x00000c30, 0xffffffff, 0x26283032,
	0, 0, 1, 0x00000c34, 0xffffffff, 0x34363840,
	0, 0, 1, 0x00000c38, 0xffffffff, 0x26283032,
	0, 0, 0, 0x00000c3c, 0xffffffff, 0x34363840,
	0, 0, 0, 0x00000c40, 0xffffffff, 0x26283032,
	0, 0, 0, 0x00000c44, 0xffffffff, 0x38402224,
	0, 0, 1, 0x00000c48, 0xffffffff, 0x30323436,
	0, 0, 1, 0x00000c4c, 0xffffffff, 0x22242628,
	0, 1, 0, 0x00000e20, 0xffffffff, 0x32343638,
	0, 1, 0, 0x00000e24, 0xffffffff, 0x36384042,
	0, 1, 0, 0x00000e28, 0xffffffff, 0x28303234,
	0, 1, 0, 0x00000e2c, 0xffffffff, 0x34363840,
	0, 1, 0, 0x00000e30, 0xffffffff, 0x26283032,
	0, 1, 1, 0x00000e34, 0xffffffff, 0x34363840,
	0, 1, 1, 0x00000e38, 0xffffffff, 0x26283032,
	0, 1, 0, 0x00000e3c, 0xffffffff, 0x34363840,
	0, 1, 0, 0x00000e40, 0xffffffff, 0x26283032,
	0, 1, 0, 0x00000e44, 0xffffffff, 0x38402224,
	0, 1, 1, 0x00000e48, 0xffffffff, 0x30323436,
	0, 1, 1, 0x00000e4c, 0xffffffff, 0x22242628,
	1, 0, 0, 0x00000c24, 0xffffffff, 0x34363840,
	1, 0, 0, 0x00000c28, 0xffffffff, 0x26283032,
	1, 0, 0, 0x00000c2c, 0xffffffff, 0x32343638,
	1, 0, 0, 0x00000c30, 0xffffffff, 0x24262830,
	1, 0, 1, 0x00000c34, 0xffffffff, 0x32343638,
	1, 0, 1, 0x00000c38, 0xffffffff, 0x24262830,
	1, 0, 0, 0x00000c3c, 0xffffffff, 0x32343638,
	1, 0, 0, 0x00000c40, 0xffffffff, 0x24262830,
	1, 0, 0, 0x00000c44, 0xffffffff, 0x36382022,
	1, 0, 1, 0x00000c48, 0xffffffff, 0x28303234,
	1, 0, 1, 0x00000c4c, 0xffffffff, 0x20222426,
	1, 1, 0, 0x00000e24, 0xffffffff, 0x34363840,
	1, 1, 0, 0x00000e28, 0xffffffff, 0x26283032,
	1, 1, 0, 0x00000e2c, 0xffffffff, 0x32343638,
	1, 1, 0, 0x00000e30, 0xffffffff, 0x24262830,
	1, 1, 1, 0x00000e34, 0xffffffff, 0x32343638,
	1, 1, 1, 0x00000e38, 0xffffffff, 0x24262830,
	1, 1, 0, 0x00000e3c, 0xffffffff, 0x32343638,
	1, 1, 0, 0x00000e40, 0xffffffff, 0x24262830,
	1, 1, 0, 0x00000e44, 0xffffffff, 0x36382022,
	1, 1, 1, 0x00000e48, 0xffffffff, 0x28303234,
	1, 1, 1, 0x00000e4c, 0xffffffff, 0x20222426
};

void
odm_read_and_config_mp_8198f_phy_reg_pg(
	struct dm_struct	*dm
)
{
	u32	i = 0;
	u32	array_len = sizeof(array_mp_8198f_phy_reg_pg)/sizeof(u32);
	u32	*array = array_mp_8198f_phy_reg_pg;

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	void	*adapter = dm->adapter;
	HAL_DATA_TYPE	*hal_data = GET_HAL_DATA(((PADAPTER)adapter));

	PlatformZeroMemory(hal_data->BufOfLinesPwrByRate, MAX_LINES_HWCONFIG_TXT*MAX_BYTES_LINE_HWCONFIG_TXT);
	hal_data->nLinesReadPwrByRate = array_len/6;
#endif

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> odm_read_and_config_mp_8198f_phy_reg_pg\n");

	dm->phy_reg_pg_version = 1;
	dm->phy_reg_pg_value_type = PHY_REG_PG_EXACT_VALUE;

	for (i = 0; i < array_len; i += 6) {
		u32	v1 = array[i];
		u32	v2 = array[i+1];
		u32	v3 = array[i+2];
		u32	v4 = array[i+3];
		u32	v5 = array[i+4];
		u32	v6 = array[i+5];

		odm_config_bb_phy_reg_pg_8198f(dm, v1, v2, v3, v4, v5, v6);

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	rsprintf((char *)hal_data->BufOfLinesPwrByRate[i/6], 100, "%s, %s, %s, 0x%X, 0x%08X, 0x%08X,",
		(v1 == 0?"2.4G":"  5G"), (v2 == 0?"A":"B"), (v3 == 0?"1Tx":"2Tx"), v4, v5, v6);
#endif
	}
}



#endif /* end of HWIMG_SUPPORT*/

