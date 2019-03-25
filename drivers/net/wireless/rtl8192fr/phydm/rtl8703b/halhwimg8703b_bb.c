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
 *****************************************************************************/

/*Image2HeaderVersion: 3.5.2*/
#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8703B_SUPPORT == 1)
static boolean
check_positive(
	struct dm_struct *dm,
	const u32	condition1,
	const u32	condition2,
	const u32	condition3,
	const u32	condition4
)
{
	u8	_board_type = ((dm->board_type & BIT(4)) >> 4) << 0 | /* _GLNA*/
			((dm->board_type & BIT(3)) >> 3) << 1 | /* _GPA*/
			((dm->board_type & BIT(7)) >> 7) << 2 | /* _ALNA*/
			((dm->board_type & BIT(6)) >> 6) << 3 | /* _APA */
			((dm->board_type & BIT(2)) >> 2) << 4 | /* _BT*/
			((dm->board_type & BIT(1)) >> 1) << 5 | /* _NGFF*/
			((dm->board_type & BIT(5)) >> 5) << 6;  /* _TRSWT*/

	u32	cond1 = condition1, cond2 = condition2, cond3 = condition3, cond4 = condition4;

	u8	cut_version_for_para = (dm->cut_version ==  ODM_CUT_A) ? 15 : dm->cut_version;
	u8	pkg_type_for_para = (dm->package_type == 0) ? 15 : dm->package_type;

	u32	driver1 = cut_version_for_para << 24 |
			(dm->support_interface & 0xF0) << 16 |
			dm->support_platform << 16 |
			pkg_type_for_para << 12 |
			(dm->support_interface & 0x0F) << 8  |
			_board_type;

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
		  "===> %s (cond1, cond2, cond3, cond4) = (0x%X 0x%X 0x%X 0x%X)\n",
		  __func__, cond1, cond2, cond3, cond4);
	PHYDM_DBG(dm, ODM_COMP_INIT,
		  "===> %s (driver1, driver2, driver3, driver4) = (0x%X 0x%X 0x%X 0x%X)\n",
		  __func__, driver1, driver2, driver3, driver4);

	PHYDM_DBG(dm, ODM_COMP_INIT,
		  "	(Platform, Interface) = (0x%X, 0x%X)\n",
		  dm->support_platform, dm->support_interface);
	PHYDM_DBG(dm, ODM_COMP_INIT,
		  "	(Board, Package) = (0x%X, 0x%X)\n", dm->board_type,
		  dm->package_type);


	/*============== value Defined Check ===============*/
	/*QFN type [15:12] and cut version [27:24] need to do value check*/

	if (((cond1 & 0x0000F000) != 0) && ((cond1 & 0x0000F000) != (driver1 & 0x0000F000)))
		return false;
	if (((cond1 & 0x0F000000) != 0) && ((cond1 & 0x0F000000) != (driver1 & 0x0F000000)))
		return false;

	/*=============== Bit Defined Check ================*/
	/* We don't care [31:28] */

	cond1 &= 0x00FF0FFF;
	driver1 &= 0x00FF0FFF;

	if ((cond1 & driver1) == cond1) {
		u32	bit_mask = 0;

		if ((cond1 & 0x0F) == 0) /* board_type is DONTCARE*/
			return true;

		if ((cond1 & BIT(0)) != 0) /*GLNA*/
			bit_mask |= 0x000000FF;
		if ((cond1 & BIT(1)) != 0) /*GPA*/
			bit_mask |= 0x0000FF00;
		if ((cond1 & BIT(2)) != 0) /*ALNA*/
			bit_mask |= 0x00FF0000;
		if ((cond1 & BIT(3)) != 0) /*APA*/
			bit_mask |= 0xFF000000;

		if (((cond2 & bit_mask) == (driver2 & bit_mask)) && ((cond4 & bit_mask) == (driver4 & bit_mask)))  /* board_type of each RF path is matched*/
			return true;
		else
			return false;
	} else
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

u32 array_mp_8703b_agc_tab[] = {
		0xC78, 0xFC000101,
		0xC78, 0xFB010101,
		0xC78, 0xFA020101,
		0xC78, 0xF9030101,
		0xC78, 0xF8040101,
		0xC78, 0xF7050101,
		0xC78, 0xF6060101,
		0xC78, 0xF5070101,
		0xC78, 0xF4080101,
		0xC78, 0xF3090101,
		0xC78, 0xF20A0101,
		0xC78, 0xF10B0101,
		0xC78, 0xF00C0101,
		0xC78, 0xEF0D0101,
		0xC78, 0xEE0E0101,
		0xC78, 0xED0F0101,
		0xC78, 0xEC100101,
		0xC78, 0xEB110101,
		0xC78, 0xEA120101,
		0xC78, 0xE9130101,
		0xC78, 0xE8140101,
		0xC78, 0xE7150101,
		0xC78, 0xE6160101,
		0xC78, 0xE5170101,
		0xC78, 0xE4180101,
		0xC78, 0xE3190101,
		0xC78, 0x661A0101,
		0xC78, 0x651B0101,
		0xC78, 0x641C0101,
		0xC78, 0x631D0101,
		0xC78, 0x071E0101,
		0xC78, 0x061F0101,
		0xC78, 0x05200101,
		0xC78, 0x04210101,
		0xC78, 0x03220101,
		0xC78, 0xE8230001,
		0xC78, 0xE7240001,
		0xC78, 0xE6250001,
		0xC78, 0xE5260001,
		0xC78, 0xE4270001,
		0xC78, 0x89280001,
		0xC78, 0x88290001,
		0xC78, 0x872A0001,
		0xC78, 0x862B0001,
		0xC78, 0x852C0001,
		0xC78, 0x482D0001,
		0xC78, 0x472E0001,
		0xC78, 0x462F0001,
		0xC78, 0x45300001,
		0xC78, 0x44310001,
		0xC78, 0x07320001,
		0xC78, 0x06330001,
		0xC78, 0x05340001,
		0xC78, 0x04350001,
		0xC78, 0x03360001,
		0xC78, 0x02370001,
		0xC78, 0x01380001,
		0xC78, 0x00390001,
		0xC78, 0x003A0001,
		0xC78, 0x003B0001,
		0xC78, 0x003C0001,
		0xC78, 0x003D0001,
		0xC78, 0x003E0001,
		0xC78, 0x003F0001,
		0xC78, 0x7F002001,
		0xC78, 0x7F012001,
		0xC78, 0x7F022001,
		0xC78, 0x7F032001,
		0xC78, 0x7F042001,
		0xC78, 0x7F052001,
		0xC78, 0x7F062001,
		0xC78, 0x7F072001,
		0xC78, 0x7F082001,
		0xC78, 0x7F092001,
		0xC78, 0x7F0A2001,
		0xC78, 0x7F0B2001,
		0xC78, 0x7F0C2001,
		0xC78, 0x7F0D2001,
		0xC78, 0x7F0E2001,
		0xC78, 0x7F0F2001,
		0xC78, 0x7F102001,
		0xC78, 0x7F112001,
		0xC78, 0x7E122001,
		0xC78, 0x7D132001,
		0xC78, 0x7C142001,
		0xC78, 0x7B152001,
		0xC78, 0x7A162001,
		0xC78, 0x79172001,
		0xC78, 0x78182001,
		0xC78, 0x77192001,
		0xC78, 0x761A2001,
		0xC78, 0x751B2001,
		0xC78, 0x741C2001,
		0xC78, 0x731D2001,
		0xC78, 0x721E2001,
		0xC78, 0x711F2001,
		0xC78, 0x70202001,
		0xC78, 0x6F212001,
		0xC78, 0x6E222001,
		0xC78, 0x6D232001,
		0xC78, 0x6C242001,
		0xC78, 0x6B252001,
		0xC78, 0x6A262001,
		0xC78, 0x69272001,
		0xC78, 0x68282001,
		0xC78, 0x67292001,
		0xC78, 0x662A2001,
		0xC78, 0x652B2001,
		0xC78, 0x642C2001,
		0xC78, 0x632D2001,
		0xC78, 0x622E2001,
		0xC78, 0x612F2001,
		0xC78, 0x60302001,
		0xC78, 0x42312001,
		0xC78, 0x41322001,
		0xC78, 0x40332001,
		0xC78, 0x23342001,
		0xC78, 0x22352001,
		0xC78, 0x21362001,
		0xC78, 0x20372001,
		0xC78, 0x00382001,
		0xC78, 0x02392001,
		0xC78, 0x013A2001,
		0xC78, 0x003B2001,
		0xC78, 0x003C2001,
		0xC78, 0x003D2001,
		0xC78, 0x003E2001,
		0xC78, 0x003F2001,
		0xC78, 0x7F003101,
		0xC78, 0x7F013101,
		0xC78, 0x7F023101,
		0xC78, 0x7F033101,
		0xC78, 0x7F043101,
		0xC78, 0x7F053101,
		0xC78, 0x7F063101,
		0xC78, 0x7E073101,
		0xC78, 0x7D083101,
		0xC78, 0x7C093101,
		0xC78, 0x7B0A3101,
		0xC78, 0x7A0B3101,
		0xC78, 0x790C3101,
		0xC78, 0x780D3101,
		0xC78, 0x770E3101,
		0xC78, 0x760F3101,
		0xC78, 0x75103101,
		0xC78, 0x74113101,
		0xC78, 0x73123101,
		0xC78, 0x72133101,
		0xC78, 0x71143101,
		0xC78, 0x70153101,
		0xC78, 0x6F163101,
		0xC78, 0x69173101,
		0xC78, 0x68183101,
		0xC78, 0x67193101,
		0xC78, 0x661A3101,
		0xC78, 0x651B3101,
		0xC78, 0x641C3101,
		0xC78, 0x631D3101,
		0xC78, 0x621E3101,
		0xC78, 0x611F3101,
		0xC78, 0x60203101,
		0xC78, 0x42213101,
		0xC78, 0x41223101,
		0xC78, 0x40233101,
		0xC78, 0x22243101,
		0xC78, 0x21253101,
		0xC78, 0x20263101,
		0xC78, 0x00273101,
		0xC78, 0x00283101,
		0xC78, 0x00293101,
		0xC78, 0x002A3101,
		0xC78, 0x002B3101,
		0xC78, 0x002C3101,
		0xC78, 0x002D3101,
		0xC78, 0x002E3101,
		0xC78, 0x002F3101,
		0xC78, 0x00303101,
		0xC78, 0x00313101,
		0xC78, 0x00323101,
		0xC78, 0x00333101,
		0xC78, 0x00343101,
		0xC78, 0x00353101,
		0xC78, 0x00363101,
		0xC78, 0x00373101,
		0xC78, 0x00383101,
		0xC78, 0x00393101,
		0xC78, 0x003A3101,
		0xC78, 0x003B3101,
		0xC78, 0x003C3101,
		0xC78, 0x003D3101,
		0xC78, 0x003E3101,
		0xC78, 0x003F3101,
		0xC78, 0xFA403101,
		0xC78, 0xF9413101,
		0xC78, 0xF8423101,
		0xC78, 0xF7433101,
		0xC78, 0xF6443101,
		0xC78, 0xF5453101,
		0xC78, 0xF4463101,
		0xC78, 0xF3473101,
		0xC78, 0xF2483101,
		0xC78, 0xE1493101,
		0xC78, 0xE04A3101,
		0xC78, 0xEF4B3101,
		0xC78, 0xEE4C3101,
		0xC78, 0xED4D3101,
		0xC78, 0xEC4E3101,
		0xC78, 0xEB4F3101,
		0xC78, 0xEA503101,
		0xC78, 0xE9513101,
		0xC78, 0xE8523101,
		0xC78, 0xE7533101,
		0xC78, 0xE6543101,
		0xC78, 0xE5553101,
		0xC78, 0xE4563101,
		0xC78, 0xE3573101,
		0xC78, 0xE2583101,
		0xC78, 0xE1593101,
		0xC78, 0xE05A3101,
		0xC78, 0xC25B3101,
		0xC78, 0xC15C3101,
		0xC78, 0xC05D3101,
		0xC78, 0x825E3101,
		0xC78, 0x815F3101,
		0xC78, 0x80603101,
		0xC78, 0x80613101,
		0xC78, 0x80623101,
		0xC78, 0x80633101,
		0xC78, 0x80643101,
		0xC78, 0x80653101,
		0xC78, 0x80663101,
		0xC78, 0x80673101,
		0xC78, 0x80683101,
		0xC78, 0x80693101,
		0xC78, 0x806A3101,
		0xC78, 0x806B3101,
		0xC78, 0x806C3101,
		0xC78, 0x806D3101,
		0xC78, 0x806E3101,
		0xC78, 0x806F3101,
		0xC78, 0x80703101,
		0xC78, 0x80713101,
		0xC78, 0x80723101,
		0xC78, 0x80733101,
		0xC78, 0x80743101,
		0xC78, 0x80753101,
		0xC78, 0x80763101,
		0xC78, 0x80773101,
		0xC78, 0x80783101,
		0xC78, 0x80793101,
		0xC78, 0x807A3101,
		0xC78, 0x807B3101,
		0xC78, 0x807C3101,
		0xC78, 0x807D3101,
		0xC78, 0x807E3101,
		0xC78, 0x807F3101,
		0xC78, 0xFF402001,
		0xC78, 0xFF412001,
		0xC78, 0xFF422001,
		0xC78, 0xFF432001,
		0xC78, 0xFF442001,
		0xC78, 0xFF452001,
		0xC78, 0xFF462001,
		0xC78, 0xFF472001,
		0xC78, 0xFF482001,
		0xC78, 0xFF492001,
		0xC78, 0xFF4A2001,
		0xC78, 0xFF4B2001,
		0xC78, 0xFF4C2001,
		0xC78, 0xFE4D2001,
		0xC78, 0xFD4E2001,
		0xC78, 0xFC4F2001,
		0xC78, 0xFB502001,
		0xC78, 0xFA512001,
		0xC78, 0xF9522001,
		0xC78, 0xF8532001,
		0xC78, 0xF7542001,
		0xC78, 0xF6552001,
		0xC78, 0xF5562001,
		0xC78, 0xF4572001,
		0xC78, 0xF3582001,
		0xC78, 0xF2592001,
		0xC78, 0xF15A2001,
		0xC78, 0xF05B2001,
		0xC78, 0xEF5C2001,
		0xC78, 0xEE5D2001,
		0xC78, 0xED5E2001,
		0xC78, 0xEC5F2001,
		0xC78, 0xEB602001,
		0xC78, 0xEA612001,
		0xC78, 0xE9622001,
		0xC78, 0xE8632001,
		0xC78, 0xE7642001,
		0xC78, 0xE6652001,
		0xC78, 0xE5662001,
		0xC78, 0xE4672001,
		0xC78, 0xE3682001,
		0xC78, 0xC5692001,
		0xC78, 0xC46A2001,
		0xC78, 0xC36B2001,
		0xC78, 0xA46C2001,
		0xC78, 0x846D2001,
		0xC78, 0x836E2001,
		0xC78, 0x826F2001,
		0xC78, 0x81702001,
		0xC78, 0x80712001,
		0xC78, 0x80722001,
		0xC78, 0x80732001,
		0xC78, 0x80742001,
		0xC78, 0x80752001,
		0xC78, 0x80762001,
		0xC78, 0x80772001,
		0xC78, 0x80782001,
		0xC78, 0x80792001,
		0xC78, 0x807A2001,
		0xC78, 0x807B2001,
		0xC78, 0x807C2001,
		0xC78, 0x807D2001,
		0xC78, 0x807E2001,
		0xC78, 0x807F2001,
		0xC50, 0x69553422,
		0xC50, 0x69553420,

};

void
odm_read_and_config_mp_8703b_agc_tab(struct dm_struct *dm)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8703b_agc_tab) / sizeof(u32);
	u32	*array = array_mp_8703b_agc_tab;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> %s\n", __func__);

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29) | BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped ? false : true;
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
				odm_config_bb_agc_8703b(dm, v1, MASKDWORD, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8703b_agc_tab(void)
{
		return 18;
}

/******************************************************************************
*                           phy_reg.TXT
******************************************************************************/

u32 array_mp_8703b_phy_reg[] = {
		0x800, 0x83045700,
		0x804, 0x00000003,
		0x808, 0x0000FC00,
		0x80C, 0x0000000A,
		0x810, 0x10001331,
		0x814, 0x020C3D10,
		0x818, 0x02200385,
		0x81C, 0x00000000,
		0x820, 0x01000100,
		0x824, 0x00390204,
		0x828, 0x00000000,
		0x82C, 0x00000000,
		0x830, 0x00000000,
		0x834, 0x00000000,
		0x838, 0x00000000,
		0x83C, 0x00000000,
		0x840, 0x00010000,
		0x844, 0x00000000,
		0x848, 0x00000000,
		0x84C, 0x00000000,
		0x850, 0x00000000,
		0x854, 0x00000000,
		0x858, 0x569A11A9,
		0x85C, 0x01000014,
		0x860, 0x66F60110,
		0x864, 0x061F0649,
		0x868, 0x00000000,
		0x86C, 0x27272700,
		0x870, 0x07000760,
		0x874, 0x25004000,
		0x878, 0x00000808,
		0x87C, 0x004F0201,
		0x880, 0xB0000B1E,
		0x884, 0x00000001,
		0x888, 0x00000000,
		0x88C, 0xCCC000C0,
		0x890, 0x00000800,
		0x894, 0xFFFFFFFE,
		0x898, 0x40302010,
		0x89C, 0x00706050,
		0x900, 0x00000000,
		0x904, 0x00000023,
		0x908, 0x00000000,
		0x90C, 0x81121111,
		0x910, 0x00000002,
		0x914, 0x00000201,
		0x948, 0x99000000,
		0x94C, 0x00000010,
		0x950, 0x20003800,
		0x954, 0x4A880000,
		0x958, 0x4BC5D87A,
		0x95C, 0x04EB9B79,
		0xA00, 0x00D047C8,
		0xA04, 0x80FF800C,
		0xA08, 0x8C838300,
		0xA0C, 0x2E7F120F,
		0xA10, 0x9500BB78,
		0xA14, 0x1114D028,
		0xA18, 0x00881117,
		0xA1C, 0x89140F00,
		0xA20, 0xD1D80000,
		0xA24, 0x5A7DA0BD,
		0xA28, 0x0000223B,
		0xA2C, 0x00D30000,
		0xA70, 0x101FBF00,
		0xA74, 0x00000007,
		0xA78, 0x00008900,
		0xA7C, 0x225B0606,
		0xA80, 0x2180FA74,
		0xA84, 0x00120000,
		0xA88, 0x040C0000,
		0xA8C, 0x12345678,
		0xA90, 0xABCDEF00,
		0xA94, 0x001B1B89,
		0xA98, 0x05100000,
		0xA9C, 0x3F000000,
		0xAA0, 0x00000000,
		0xB2C, 0x00000000,
		0xC00, 0x48071D40,
		0xC04, 0x03A05611,
		0xC08, 0x000000E4,
		0xC0C, 0x6C6C6C6C,
		0xC10, 0x18800000,
		0xC14, 0x40000100,
		0xC18, 0x08800000,
		0xC1C, 0x40000100,
		0xC20, 0x00000000,
		0xC24, 0x00000000,
		0xC28, 0x00000000,
		0xC2C, 0x00000000,
		0xC30, 0x69E9AC4B,
		0xC34, 0x31000040,
		0xC38, 0x21688080,
		0xC3C, 0x000016CC,
		0xC40, 0x1F78403F,
		0xC44, 0x00010036,
		0xC48, 0xEC020107,
		0xC4C, 0x007F037F,
		0xC50, 0x69553420,
		0xC54, 0x43BC0094,
		0xC58, 0x00015967,
		0xC5C, 0x18250492,
		0xC60, 0x00000000,
		0xC64, 0x7112848B,
		0xC68, 0x47C07BFF,
		0xC6C, 0x00000036,
		0xC70, 0x2C7F000D,
		0xC74, 0x020600DB,
		0xC78, 0x0000001F,
		0xC7C, 0x00B91612,
		0xC80, 0x390000E4,
		0xC84, 0x19F60000,
		0xC88, 0x40000100,
		0xC8C, 0x20200000,
		0xC90, 0x00091521,
		0xC94, 0x00000000,
		0xC98, 0x00121820,
		0xC9C, 0x00007F7F,
		0xCA0, 0x00000000,
		0xCA4, 0x000300A0,
		0xCA8, 0x00000000,
		0xCAC, 0x00000000,
		0xCB0, 0x00000000,
		0xCB4, 0x00000000,
		0xCB8, 0x00000000,
		0xCBC, 0x28000000,
		0xCC0, 0x00000000,
		0xCC4, 0x00000000,
		0xCC8, 0x00000000,
		0xCCC, 0x00000000,
		0xCD0, 0x00000000,
		0xCD4, 0x00000000,
		0xCD8, 0x64B22427,
		0xCDC, 0x00766932,
		0xCE0, 0x00222222,
		0xCE4, 0x10000000,
		0xCE8, 0x37644302,
		0xCEC, 0x2F97D40C,
		0xD00, 0x00030740,
		0xD04, 0x40020401,
		0xD08, 0x0000907F,
		0xD0C, 0x20010201,
		0xD10, 0xA0633333,
		0xD14, 0x3333BC53,
		0xD18, 0x7A8F5B6F,
		0xD2C, 0xCB979975,
		0xD30, 0x00000000,
		0xD34, 0x80608000,
		0xD38, 0x98000000,
		0xD3C, 0x40127353,
		0xD40, 0x00000000,
		0xD44, 0x00000000,
		0xD48, 0x00000000,
		0xD4C, 0x00000000,
		0xD50, 0x6437140A,
		0xD54, 0x00000000,
		0xD58, 0x00000282,
		0xD5C, 0x30032064,
		0xD60, 0x4653DE68,
		0xD64, 0x04518A3C,
		0xD68, 0x00002101,
		0xE00, 0x2D2D2D2D,
		0xE04, 0x2D2D2D2D,
		0xE08, 0x0390272D,
		0xE10, 0x2D2D2D2D,
		0xE14, 0x2D2D2D2D,
		0xE18, 0x2D2D2D2D,
		0xE1C, 0x2D2D2D2D,
		0xE28, 0x00000000,
		0xE30, 0x1000DC1F,
		0xE34, 0x10008C1F,
		0xE38, 0x02140102,
		0xE3C, 0x681604C2,
		0xE40, 0x01007C00,
		0xE44, 0x01004800,
		0xE48, 0xFB000000,
		0xE4C, 0x000028D1,
		0xE50, 0x1000DC1F,
		0xE54, 0x10008C1F,
		0xE58, 0x02140102,
		0xE5C, 0x28160D05,
		0xE60, 0x00000048,
		0xE68, 0x001B25A4,
		0xE6C, 0x01C00014,
		0xE70, 0x01C00014,
		0xE74, 0x02000014,
		0xE78, 0x02000014,
		0xE7C, 0x02000014,
		0xE80, 0x02000014,
		0xE84, 0x01C00014,
		0xE88, 0x02000014,
		0xE8C, 0x01C00014,
		0xED0, 0x01C00014,
		0xED4, 0x01C00014,
		0xED8, 0x01C00014,
		0xEDC, 0x00000014,
		0xEE0, 0x00000014,
		0xEE8, 0x21555448,
		0xEEC, 0x03C00014,
		0xF14, 0x00000003,
		0xF4C, 0x00000000,
		0xF00, 0x00000300,

};

void
odm_read_and_config_mp_8703b_phy_reg(struct dm_struct *dm)
{
	u32	i = 0;
	u8	c_cond;
	boolean	is_matched = true, is_skipped = false;
	u32	array_len = sizeof(array_mp_8703b_phy_reg) / sizeof(u32);
	u32	*array = array_mp_8703b_phy_reg;

	u32	v1 = 0, v2 = 0, pre_v1 = 0, pre_v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> %s\n", __func__);

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];

		if (v1 & (BIT(31) | BIT(30))) {/*positive & negative condition*/
			if (v1 & BIT(31)) {/* positive condition*/
				c_cond  = (u8)((v1 & (BIT(29) | BIT(28))) >> 28);
				if (c_cond == COND_ENDIF) {/*end*/
					is_matched = true;
					is_skipped = false;
					PHYDM_DBG(dm, ODM_COMP_INIT, "ENDIF\n");
				} else if (c_cond == COND_ELSE) { /*else*/
					is_matched = is_skipped ? false : true;
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
				odm_config_bb_phy_8703b(dm, v1, MASKDWORD, v2);
		}
		i = i + 2;
	}
}

u32
odm_get_version_mp_8703b_phy_reg(void)
{
		return 18;
}

/******************************************************************************
*                           phy_reg_pg.TXT
******************************************************************************/

u32 array_mp_8703b_phy_reg_pg[] = {
	0, 0, 0, 0x00000e08, 0x0000ff00, 0x00003200,
	0, 0, 0, 0x0000086c, 0xffffff00, 0x32323200,
	0, 0, 0, 0x00000e00, 0xffffffff, 0x34363636,
	0, 0, 0, 0x00000e04, 0xffffffff, 0x28303234,
	0, 0, 0, 0x00000e10, 0xffffffff, 0x30343434,
	0, 0, 0, 0x00000e14, 0xffffffff, 0x26262830
};

void
odm_read_and_config_mp_8703b_phy_reg_pg(struct dm_struct *dm)
{
	u32	i = 0;
	u32	array_len = sizeof(array_mp_8703b_phy_reg_pg) / sizeof(u32);
	u32	*array = array_mp_8703b_phy_reg_pg;

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	void	*adapter = dm->adapter;
	HAL_DATA_TYPE	*hal_data = GET_HAL_DATA(((PADAPTER)adapter));

	PlatformZeroMemory(hal_data->BufOfLinesPwrByRate, MAX_LINES_HWCONFIG_TXT * MAX_BYTES_LINE_HWCONFIG_TXT);
	hal_data->nLinesReadPwrByRate = array_len / 6;
#endif

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> %s\n", __func__);

	dm->phy_reg_pg_version = 1;
	dm->phy_reg_pg_value_type = PHY_REG_PG_EXACT_VALUE;

	for (i = 0; i < array_len; i += 6) {
		u32	v1 = array[i];
		u32	v2 = array[i + 1];
		u32	v3 = array[i + 2];
		u32	v4 = array[i + 3];
		u32	v5 = array[i + 4];
		u32	v6 = array[i + 5];

		odm_config_bb_phy_reg_pg_8703b(dm, v1, v2, v3, v4, v5, v6);

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	rsprintf((char *)hal_data->BufOfLinesPwrByRate[i / 6], 100, "%s, %s, %s, 0x%X, 0x%08X, 0x%08X,",
		(v1 == 0 ? "2.4G" : "  5G"), (v2 == 0 ? "A" : "B"), (v3 == 0 ? "1Tx" : "2Tx"), v4, v5, v6);
#endif
	}
}



#endif /* end of HWIMG_SUPPORT*/

