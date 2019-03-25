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

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if 1
u32 array_mp_8195b_cal_init[] = {
	0x1b00, 0x00000000,
	0x1b80, 0x00000007,
	0x1b80, 0x090a0005,
	0x1b80, 0x090a0007,
	0x1b80, 0x0ffe0015,
	0x1b80, 0x0ffe0017,
	0x1b80, 0x00220025,
	0x1b80, 0x00220027,
	0x1b80, 0x00040035,
	0x1b80, 0x00040037,
	0x1b80, 0x05c00045,
	0x1b80, 0x05c00047,
	0x1b80, 0x00070055,
	0x1b80, 0x00070057,
	0x1b80, 0x64000065,
	0x1b80, 0x64000067,
	0x1b80, 0x00020075,
	0x1b80, 0x00020077,
	0x1b80, 0x00080085,
	0x1b80, 0x00080087,
	0x1b80, 0x80000095,
	0x1b80, 0x80000097,
	0x1b80, 0x090800a5,
	0x1b80, 0x090800a7,
	0x1b80, 0x0f0200b5,
	0x1b80, 0x0f0200b7,
	0x1b80, 0x002200c5,
	0x1b80, 0x002200c7,
	0x1b80, 0x000400d5,
	0x1b80, 0x000400d7,
	0x1b80, 0x05c000e5,
	0x1b80, 0x05c000e7,
	0x1b80, 0x000700f5,
	0x1b80, 0x000700f7,
	0x1b80, 0x64020105,
	0x1b80, 0x64020107,
	0x1b80, 0x00020115,
	0x1b80, 0x00020117,
	0x1b80, 0x00040125,
	0x1b80, 0x00040127,
	0x1b80, 0x4a000135,
	0x1b80, 0x4a000137,
	0x1b80, 0x4b040145,
	0x1b80, 0x4b040147,
	0x1b80, 0x86030155,
	0x1b80, 0x86030157,
	0x1b80, 0x40090165,
	0x1b80, 0x40090167,
	0x1b80, 0xe0220175,
	0x1b80, 0xe0220177,
	0x1b80, 0x4b050185,
	0x1b80, 0x4b050187,
	0x1b80, 0x87030195,
	0x1b80, 0x87030197,
	0x1b80, 0x400b01a5,
	0x1b80, 0x400b01a7,
	0x1b80, 0xe02201b5,
	0x1b80, 0xe02201b7,
	0x1b80, 0x4b0001c5,
	0x1b80, 0x4b0001c7,
	0x1b80, 0x000701d5,
	0x1b80, 0x000701d7,
	0x1b80, 0x4c0001e5,
	0x1b80, 0x4c0001e7,
	0x1b80, 0x000401f5,
	0x1b80, 0x000401f7,
	0x1b80, 0x50550205,
	0x1b80, 0x50550207,
	0x1b80, 0x30000215,
	0x1b80, 0x30000217,
	0x1b80, 0xe1f10225,
	0x1b80, 0xe1f10227,
	0x1b80, 0xa5110235,
	0x1b80, 0xa5110237,
	0x1b80, 0xf0120245,
	0x1b80, 0xf0120247,
	0x1b80, 0xf1120255,
	0x1b80, 0xf1120257,
	0x1b80, 0xf2120265,
	0x1b80, 0xf2120267,
	0x1b80, 0xf3120275,
	0x1b80, 0xf3120277,
	0x1b80, 0xf4120285,
	0x1b80, 0xf4120287,
	0x1b80, 0xf5120295,
	0x1b80, 0xf5120297,
	0x1b80, 0xf61202a5,
	0x1b80, 0xf61202a7,
	0x1b80, 0xf71202b5,
	0x1b80, 0xf71202b7,
	0x1b80, 0xf81202c5,
	0x1b80, 0xf81202c7,
	0x1b80, 0xf91202d5,
	0x1b80, 0xf91202d7,
	0x1b80, 0xfa1202e5,
	0x1b80, 0xfa1202e7,
	0x1b80, 0xfb1202f5,
	0x1b80, 0xfb1202f7,
	0x1b80, 0xfc120305,
	0x1b80, 0xfc120307,
	0x1b80, 0xfd120315,
	0x1b80, 0xfd120317,
	0x1b80, 0xfe120325,
	0x1b80, 0xfe120327,
	0x1b80, 0xff120335,
	0x1b80, 0xff120337,
	0x1b80, 0xf0120345,
	0x1b80, 0xf0120347,
	0x1b80, 0x00010355,
	0x1b80, 0x00010357,
	0x1b80, 0x304a0365,
	0x1b80, 0x304a0367,
	0x1b80, 0x30770375,
	0x1b80, 0x30770377,
	0x1b80, 0x30c60385,
	0x1b80, 0x30c60387,
	0x1b80, 0x30c90395,
	0x1b80, 0x30c90397,
	0x1b80, 0x307903a5,
	0x1b80, 0x307903a7,
	0x1b80, 0x308403b5,
	0x1b80, 0x308403b7,
	0x1b80, 0x308f03c5,
	0x1b80, 0x308f03c7,
	0x1b80, 0x30d403d5,
	0x1b80, 0x30d403d7,
	0x1b80, 0x30cd03e5,
	0x1b80, 0x30cd03e7,
	0x1b80, 0x30e403f5,
	0x1b80, 0x30e403f7,
	0x1b80, 0x30ef0405,
	0x1b80, 0x30ef0407,
	0x1b80, 0x30fa0415,
	0x1b80, 0x30fa0417,
	0x1b80, 0x30470425,
	0x1b80, 0x30470427,
	0x1b80, 0x312d0435,
	0x1b80, 0x312d0437,
	0x1b80, 0x313e0445,
	0x1b80, 0x313e0447,
	0x1b80, 0x31530455,
	0x1b80, 0x31530457,
	0x1b80, 0x30620465,
	0x1b80, 0x30620467,
	0x1b80, 0x00040475,
	0x1b80, 0x00040477,
	0x1b80, 0x50660485,
	0x1b80, 0x50660487,
	0x1b80, 0x30000495,
	0x1b80, 0x30000497,
	0x1b80, 0xe17e04a5,
	0x1b80, 0xe17e04a7,
	0x1b80, 0x4d0404b5,
	0x1b80, 0x4d0404b7,
	0x1b80, 0x208004c5,
	0x1b80, 0x208004c7,
	0x1b80, 0x000004d5,
	0x1b80, 0x000004d7,
	0x1b80, 0x4d0004e5,
	0x1b80, 0x4d0004e7,
	0x1b80, 0x550704f5,
	0x1b80, 0x550704f7,
	0x1b80, 0xe1760505,
	0x1b80, 0xe1760507,
	0x1b80, 0xe1760515,
	0x1b80, 0xe1760517,
	0x1b80, 0x4d040525,
	0x1b80, 0x4d040527,
	0x1b80, 0x20880535,
	0x1b80, 0x20880537,
	0x1b80, 0x02000545,
	0x1b80, 0x02000547,
	0x1b80, 0x4d000555,
	0x1b80, 0x4d000557,
	0x1b80, 0x550f0565,
	0x1b80, 0x550f0567,
	0x1b80, 0xe1760575,
	0x1b80, 0xe1760577,
	0x1b80, 0x4f020585,
	0x1b80, 0x4f020587,
	0x1b80, 0x4e000595,
	0x1b80, 0x4e000597,
	0x1b80, 0x530205a5,
	0x1b80, 0x530205a7,
	0x1b80, 0x520105b5,
	0x1b80, 0x520105b7,
	0x1b80, 0xe17a05c5,
	0x1b80, 0xe17a05c7,
	0x1b80, 0x4d0805d5,
	0x1b80, 0x4d0805d7,
	0x1b80, 0x571005e5,
	0x1b80, 0x571005e7,
	0x1b80, 0x570005f5,
	0x1b80, 0x570005f7,
	0x1b80, 0x4d000605,
	0x1b80, 0x4d000607,
	0x1b80, 0x00010615,
	0x1b80, 0x00010617,
	0x1b80, 0x62060625,
	0x1b80, 0x62060627,
	0x1b80, 0xe17e0635,
	0x1b80, 0xe17e0637,
	0x1b80, 0x4d040645,
	0x1b80, 0x4d040647,
	0x1b80, 0x20800655,
	0x1b80, 0x20800657,
	0x1b80, 0x00000665,
	0x1b80, 0x00000667,
	0x1b80, 0x4d000675,
	0x1b80, 0x4d000677,
	0x1b80, 0x55070685,
	0x1b80, 0x55070687,
	0x1b80, 0xe1760695,
	0x1b80, 0xe1760697,
	0x1b80, 0xe17606a5,
	0x1b80, 0xe17606a7,
	0x1b80, 0x4d0406b5,
	0x1b80, 0x4d0406b7,
	0x1b80, 0x208806c5,
	0x1b80, 0x208806c7,
	0x1b80, 0x020006d5,
	0x1b80, 0x020006d7,
	0x1b80, 0x4d0006e5,
	0x1b80, 0x4d0006e7,
	0x1b80, 0x550f06f5,
	0x1b80, 0x550f06f7,
	0x1b80, 0xe1760705,
	0x1b80, 0xe1760707,
	0x1b80, 0x4f020715,
	0x1b80, 0x4f020717,
	0x1b80, 0x4e000725,
	0x1b80, 0x4e000727,
	0x1b80, 0x53020735,
	0x1b80, 0x53020737,
	0x1b80, 0x52010745,
	0x1b80, 0x52010747,
	0x1b80, 0xe17a0755,
	0x1b80, 0xe17a0757,
	0x1b80, 0x305d0765,
	0x1b80, 0x305d0767,
	0x1b80, 0xe17e0775,
	0x1b80, 0xe17e0777,
	0x1b80, 0x00010785,
	0x1b80, 0x00010787,
	0x1b80, 0x309b0795,
	0x1b80, 0x309b0797,
	0x1b80, 0x002307a5,
	0x1b80, 0x002307a7,
	0x1b80, 0xe1e407b5,
	0x1b80, 0xe1e407b7,
	0x1b80, 0x000207c5,
	0x1b80, 0x000207c7,
	0x1b80, 0x54e907d5,
	0x1b80, 0x54e907d7,
	0x1b80, 0x0ba607e5,
	0x1b80, 0x0ba607e7,
	0x1b80, 0x002307f5,
	0x1b80, 0x002307f7,
	0x1b80, 0xe1e40805,
	0x1b80, 0xe1e40807,
	0x1b80, 0x00020815,
	0x1b80, 0x00020817,
	0x1b80, 0x4d300825,
	0x1b80, 0x4d300827,
	0x1b80, 0x30b60835,
	0x1b80, 0x30b60837,
	0x1b80, 0x30960845,
	0x1b80, 0x30960847,
	0x1b80, 0x00220855,
	0x1b80, 0x00220857,
	0x1b80, 0xe1e40865,
	0x1b80, 0xe1e40867,
	0x1b80, 0x00020875,
	0x1b80, 0x00020877,
	0x1b80, 0x54e80885,
	0x1b80, 0x54e80887,
	0x1b80, 0x0ba60895,
	0x1b80, 0x0ba60897,
	0x1b80, 0x002208a5,
	0x1b80, 0x002208a7,
	0x1b80, 0xe1e408b5,
	0x1b80, 0xe1e408b7,
	0x1b80, 0x000208c5,
	0x1b80, 0x000208c7,
	0x1b80, 0x4d3008d5,
	0x1b80, 0x4d3008d7,
	0x1b80, 0x30b608e5,
	0x1b80, 0x30b608e7,
	0x1b80, 0x6c1808f5,
	0x1b80, 0x6c1808f7,
	0x1b80, 0x6d0f0905,
	0x1b80, 0x6d0f0907,
	0x1b80, 0xe17e0915,
	0x1b80, 0xe17e0917,
	0x1b80, 0xe1e40925,
	0x1b80, 0xe1e40927,
	0x1b80, 0x6c480935,
	0x1b80, 0x6c480937,
	0x1b80, 0xe17e0945,
	0x1b80, 0xe17e0947,
	0x1b80, 0xe1e40955,
	0x1b80, 0xe1e40957,
	0x1b80, 0x0ba80965,
	0x1b80, 0x0ba80967,
	0x1b80, 0x6c880975,
	0x1b80, 0x6c880977,
	0x1b80, 0x6d0f0985,
	0x1b80, 0x6d0f0987,
	0x1b80, 0xe17e0995,
	0x1b80, 0xe17e0997,
	0x1b80, 0xe1e409a5,
	0x1b80, 0xe1e409a7,
	0x1b80, 0x0ba909b5,
	0x1b80, 0x0ba909b7,
	0x1b80, 0x6cc809c5,
	0x1b80, 0x6cc809c7,
	0x1b80, 0x6d0f09d5,
	0x1b80, 0x6d0f09d7,
	0x1b80, 0xe17e09e5,
	0x1b80, 0xe17e09e7,
	0x1b80, 0xe1e409f5,
	0x1b80, 0xe1e409f7,
	0x1b80, 0x6cf80a05,
	0x1b80, 0x6cf80a07,
	0x1b80, 0xe17e0a15,
	0x1b80, 0xe17e0a17,
	0x1b80, 0xe1e40a25,
	0x1b80, 0xe1e40a27,
	0x1b80, 0x6c080a35,
	0x1b80, 0x6c080a37,
	0x1b80, 0x6d000a45,
	0x1b80, 0x6d000a47,
	0x1b80, 0xe17e0a55,
	0x1b80, 0xe17e0a57,
	0x1b80, 0xe1e40a65,
	0x1b80, 0xe1e40a67,
	0x1b80, 0x6c380a75,
	0x1b80, 0x6c380a77,
	0x1b80, 0xe17e0a85,
	0x1b80, 0xe17e0a87,
	0x1b80, 0xe1e40a95,
	0x1b80, 0xe1e40a97,
	0x1b80, 0xf4d00aa5,
	0x1b80, 0xf4d00aa7,
	0x1b80, 0x6c780ab5,
	0x1b80, 0x6c780ab7,
	0x1b80, 0xe17e0ac5,
	0x1b80, 0xe17e0ac7,
	0x1b80, 0xe1e40ad5,
	0x1b80, 0xe1e40ad7,
	0x1b80, 0xf5d70ae5,
	0x1b80, 0xf5d70ae7,
	0x1b80, 0x6cb80af5,
	0x1b80, 0x6cb80af7,
	0x1b80, 0xe17e0b05,
	0x1b80, 0xe17e0b07,
	0x1b80, 0xe1e40b15,
	0x1b80, 0xe1e40b17,
	0x1b80, 0x6ce80b25,
	0x1b80, 0x6ce80b27,
	0x1b80, 0xe17e0b35,
	0x1b80, 0xe17e0b37,
	0x1b80, 0xe1e40b45,
	0x1b80, 0xe1e40b47,
	0x1b80, 0x4d300b55,
	0x1b80, 0x4d300b57,
	0x1b80, 0x55010b65,
	0x1b80, 0x55010b67,
	0x1b80, 0x57040b75,
	0x1b80, 0x57040b77,
	0x1b80, 0x57000b85,
	0x1b80, 0x57000b87,
	0x1b80, 0x96000b95,
	0x1b80, 0x96000b97,
	0x1b80, 0x57080ba5,
	0x1b80, 0x57080ba7,
	0x1b80, 0x57000bb5,
	0x1b80, 0x57000bb7,
	0x1b80, 0x95000bc5,
	0x1b80, 0x95000bc7,
	0x1b80, 0x4d000bd5,
	0x1b80, 0x4d000bd7,
	0x1b80, 0x63070be5,
	0x1b80, 0x63070be7,
	0x1b80, 0x7b200bf5,
	0x1b80, 0x7b200bf7,
	0x1b80, 0x7a000c05,
	0x1b80, 0x7a000c07,
	0x1b80, 0x79000c15,
	0x1b80, 0x79000c17,
	0x1b80, 0x7f200c25,
	0x1b80, 0x7f200c27,
	0x1b80, 0x7e000c35,
	0x1b80, 0x7e000c37,
	0x1b80, 0x7d000c45,
	0x1b80, 0x7d000c47,
	0x1b80, 0x00010c55,
	0x1b80, 0x00010c57,
	0x1b80, 0x62060c65,
	0x1b80, 0x62060c67,
	0x1b80, 0xe17e0c75,
	0x1b80, 0xe17e0c77,
	0x1b80, 0x00010c85,
	0x1b80, 0x00010c87,
	0x1b80, 0x5c320c95,
	0x1b80, 0x5c320c97,
	0x1b80, 0xe1e00ca5,
	0x1b80, 0xe1e00ca7,
	0x1b80, 0xe1ac0cb5,
	0x1b80, 0xe1ac0cb7,
	0x1b80, 0x00010cc5,
	0x1b80, 0x00010cc7,
	0x1b80, 0x5c320cd5,
	0x1b80, 0x5c320cd7,
	0x1b80, 0x6c480ce5,
	0x1b80, 0x6c480ce7,
	0x1b80, 0x6d0f0cf5,
	0x1b80, 0x6d0f0cf7,
	0x1b80, 0x62060d05,
	0x1b80, 0x62060d07,
	0x1b80, 0x0bb00d15,
	0x1b80, 0x0bb00d17,
	0x1b80, 0xe17e0d25,
	0x1b80, 0xe17e0d27,
	0x1b80, 0xe1e40d35,
	0x1b80, 0xe1e40d37,
	0x1b80, 0x5c320d45,
	0x1b80, 0x5c320d47,
	0x1b80, 0x6cc80d55,
	0x1b80, 0x6cc80d57,
	0x1b80, 0x6d0f0d65,
	0x1b80, 0x6d0f0d67,
	0x1b80, 0x62060d75,
	0x1b80, 0x62060d77,
	0x1b80, 0x0bb10d85,
	0x1b80, 0x0bb10d87,
	0x1b80, 0xe17e0d95,
	0x1b80, 0xe17e0d97,
	0x1b80, 0xe1e40da5,
	0x1b80, 0xe1e40da7,
	0x1b80, 0x6c380db5,
	0x1b80, 0x6c380db7,
	0x1b80, 0x6d000dc5,
	0x1b80, 0x6d000dc7,
	0x1b80, 0xe17e0dd5,
	0x1b80, 0xe17e0dd7,
	0x1b80, 0xe1e40de5,
	0x1b80, 0xe1e40de7,
	0x1b80, 0xf7040df5,
	0x1b80, 0xf7040df7,
	0x1b80, 0x6cb80e05,
	0x1b80, 0x6cb80e07,
	0x1b80, 0xe17e0e15,
	0x1b80, 0xe17e0e17,
	0x1b80, 0xe1e40e25,
	0x1b80, 0xe1e40e27,
	0x1b80, 0x00010e35,
	0x1b80, 0x00010e37,
	0x1b80, 0x310a0e45,
	0x1b80, 0x310a0e47,
	0x1b80, 0x00230e55,
	0x1b80, 0x00230e57,
	0x1b80, 0xe1e90e65,
	0x1b80, 0xe1e90e67,
	0x1b80, 0x00020e75,
	0x1b80, 0x00020e77,
	0x1b80, 0x54e90e85,
	0x1b80, 0x54e90e87,
	0x1b80, 0x0ba60e95,
	0x1b80, 0x0ba60e97,
	0x1b80, 0x00230ea5,
	0x1b80, 0x00230ea7,
	0x1b80, 0xe1e90eb5,
	0x1b80, 0xe1e90eb7,
	0x1b80, 0x00020ec5,
	0x1b80, 0x00020ec7,
	0x1b80, 0x4d100ed5,
	0x1b80, 0x4d100ed7,
	0x1b80, 0x30b60ee5,
	0x1b80, 0x30b60ee7,
	0x1b80, 0x31030ef5,
	0x1b80, 0x31030ef7,
	0x1b80, 0x00220f05,
	0x1b80, 0x00220f07,
	0x1b80, 0xe1e90f15,
	0x1b80, 0xe1e90f17,
	0x1b80, 0x00020f25,
	0x1b80, 0x00020f27,
	0x1b80, 0x54e80f35,
	0x1b80, 0x54e80f37,
	0x1b80, 0x0ba60f45,
	0x1b80, 0x0ba60f47,
	0x1b80, 0x00220f55,
	0x1b80, 0x00220f57,
	0x1b80, 0xe1e90f65,
	0x1b80, 0xe1e90f67,
	0x1b80, 0x00020f75,
	0x1b80, 0x00020f77,
	0x1b80, 0x4d100f85,
	0x1b80, 0x4d100f87,
	0x1b80, 0x30b60f95,
	0x1b80, 0x30b60f97,
	0x1b80, 0x5c320fa5,
	0x1b80, 0x5c320fa7,
	0x1b80, 0x54f00fb5,
	0x1b80, 0x54f00fb7,
	0x1b80, 0x6e180fc5,
	0x1b80, 0x6e180fc7,
	0x1b80, 0x6f0f0fd5,
	0x1b80, 0x6f0f0fd7,
	0x1b80, 0xe1ac0fe5,
	0x1b80, 0xe1ac0fe7,
	0x1b80, 0xe1e90ff5,
	0x1b80, 0xe1e90ff7,
	0x1b80, 0x6e481005,
	0x1b80, 0x6e481007,
	0x1b80, 0xe1ac1015,
	0x1b80, 0xe1ac1017,
	0x1b80, 0xe1e91025,
	0x1b80, 0xe1e91027,
	0x1b80, 0x5c321035,
	0x1b80, 0x5c321037,
	0x1b80, 0x54f11045,
	0x1b80, 0x54f11047,
	0x1b80, 0x0ba81055,
	0x1b80, 0x0ba81057,
	0x1b80, 0x6e881065,
	0x1b80, 0x6e881067,
	0x1b80, 0x6f0f1075,
	0x1b80, 0x6f0f1077,
	0x1b80, 0xe1ac1085,
	0x1b80, 0xe1ac1087,
	0x1b80, 0xe1e91095,
	0x1b80, 0xe1e91097,
	0x1b80, 0x5c3210a5,
	0x1b80, 0x5c3210a7,
	0x1b80, 0x54f110b5,
	0x1b80, 0x54f110b7,
	0x1b80, 0x0ba910c5,
	0x1b80, 0x0ba910c7,
	0x1b80, 0x6ec810d5,
	0x1b80, 0x6ec810d7,
	0x1b80, 0x6f0f10e5,
	0x1b80, 0x6f0f10e7,
	0x1b80, 0xe1ac10f5,
	0x1b80, 0xe1ac10f7,
	0x1b80, 0xe1e91105,
	0x1b80, 0xe1e91107,
	0x1b80, 0x6ef81115,
	0x1b80, 0x6ef81117,
	0x1b80, 0xe1ac1125,
	0x1b80, 0xe1ac1127,
	0x1b80, 0xe1e91135,
	0x1b80, 0xe1e91137,
	0x1b80, 0x5c321145,
	0x1b80, 0x5c321147,
	0x1b80, 0x54f21155,
	0x1b80, 0x54f21157,
	0x1b80, 0x6e081165,
	0x1b80, 0x6e081167,
	0x1b80, 0x6f001175,
	0x1b80, 0x6f001177,
	0x1b80, 0xe1ac1185,
	0x1b80, 0xe1ac1187,
	0x1b80, 0xe1e91195,
	0x1b80, 0xe1e91197,
	0x1b80, 0x6e3811a5,
	0x1b80, 0x6e3811a7,
	0x1b80, 0xe1ac11b5,
	0x1b80, 0xe1ac11b7,
	0x1b80, 0xe1e911c5,
	0x1b80, 0xe1e911c7,
	0x1b80, 0xf9c811d5,
	0x1b80, 0xf9c811d7,
	0x1b80, 0x6e7811e5,
	0x1b80, 0x6e7811e7,
	0x1b80, 0xe1ac11f5,
	0x1b80, 0xe1ac11f7,
	0x1b80, 0xe1e91205,
	0x1b80, 0xe1e91207,
	0x1b80, 0xfacf1215,
	0x1b80, 0xfacf1217,
	0x1b80, 0x5c321225,
	0x1b80, 0x5c321227,
	0x1b80, 0x54f31235,
	0x1b80, 0x54f31237,
	0x1b80, 0x6eb81245,
	0x1b80, 0x6eb81247,
	0x1b80, 0xe1ac1255,
	0x1b80, 0xe1ac1257,
	0x1b80, 0xe1e91265,
	0x1b80, 0xe1e91267,
	0x1b80, 0x6ee81275,
	0x1b80, 0x6ee81277,
	0x1b80, 0xe1ac1285,
	0x1b80, 0xe1ac1287,
	0x1b80, 0xe1e91295,
	0x1b80, 0xe1e91297,
	0x1b80, 0x4d1012a5,
	0x1b80, 0x4d1012a7,
	0x1b80, 0x30b612b5,
	0x1b80, 0x30b612b7,
	0x1b80, 0x000112c5,
	0x1b80, 0x000112c7,
	0x1b80, 0x6c0012d5,
	0x1b80, 0x6c0012d7,
	0x1b80, 0x000612e5,
	0x1b80, 0x000612e7,
	0x1b80, 0x530012f5,
	0x1b80, 0x530012f7,
	0x1b80, 0x57f71305,
	0x1b80, 0x57f71307,
	0x1b80, 0x58211315,
	0x1b80, 0x58211317,
	0x1b80, 0x592e1325,
	0x1b80, 0x592e1327,
	0x1b80, 0x5a381335,
	0x1b80, 0x5a381337,
	0x1b80, 0x5b411345,
	0x1b80, 0x5b411347,
	0x1b80, 0x00071355,
	0x1b80, 0x00071357,
	0x1b80, 0x5c001365,
	0x1b80, 0x5c001367,
	0x1b80, 0x4b001375,
	0x1b80, 0x4b001377,
	0x1b80, 0x4e8f1385,
	0x1b80, 0x4e8f1387,
	0x1b80, 0x4f151395,
	0x1b80, 0x4f151397,
	0x1b80, 0x000413a5,
	0x1b80, 0x000413a7,
	0x1b80, 0xe1ce13b5,
	0x1b80, 0xe1ce13b7,
	0x1b80, 0xab0013c5,
	0x1b80, 0xab0013c7,
	0x1b80, 0x000113d5,
	0x1b80, 0x000113d7,
	0x1b80, 0x6c0013e5,
	0x1b80, 0x6c0013e7,
	0x1b80, 0x000613f5,
	0x1b80, 0x000613f7,
	0x1b80, 0x53001405,
	0x1b80, 0x53001407,
	0x1b80, 0x57f71415,
	0x1b80, 0x57f71417,
	0x1b80, 0x58211425,
	0x1b80, 0x58211427,
	0x1b80, 0x592e1435,
	0x1b80, 0x592e1437,
	0x1b80, 0x5a381445,
	0x1b80, 0x5a381447,
	0x1b80, 0x5b411455,
	0x1b80, 0x5b411457,
	0x1b80, 0x00071465,
	0x1b80, 0x00071467,
	0x1b80, 0x5c001475,
	0x1b80, 0x5c001477,
	0x1b80, 0x4b401485,
	0x1b80, 0x4b401487,
	0x1b80, 0x4e971495,
	0x1b80, 0x4e971497,
	0x1b80, 0x4f1114a5,
	0x1b80, 0x4f1114a7,
	0x1b80, 0x000414b5,
	0x1b80, 0x000414b7,
	0x1b80, 0xe1ce14c5,
	0x1b80, 0xe1ce14c7,
	0x1b80, 0xab0014d5,
	0x1b80, 0xab0014d7,
	0x1b80, 0x8b0014e5,
	0x1b80, 0x8b0014e7,
	0x1b80, 0xab0014f5,
	0x1b80, 0xab0014f7,
	0x1b80, 0x8a191505,
	0x1b80, 0x8a191507,
	0x1b80, 0x301d1515,
	0x1b80, 0x301d1517,
	0x1b80, 0x00011525,
	0x1b80, 0x00011527,
	0x1b80, 0x6c011535,
	0x1b80, 0x6c011537,
	0x1b80, 0x00061545,
	0x1b80, 0x00061547,
	0x1b80, 0x53011555,
	0x1b80, 0x53011557,
	0x1b80, 0x57f71565,
	0x1b80, 0x57f71567,
	0x1b80, 0x58211575,
	0x1b80, 0x58211577,
	0x1b80, 0x592e1585,
	0x1b80, 0x592e1587,
	0x1b80, 0x5a381595,
	0x1b80, 0x5a381597,
	0x1b80, 0x5b4115a5,
	0x1b80, 0x5b4115a7,
	0x1b80, 0x000715b5,
	0x1b80, 0x000715b7,
	0x1b80, 0x5c0015c5,
	0x1b80, 0x5c0015c7,
	0x1b80, 0x4b0015d5,
	0x1b80, 0x4b0015d7,
	0x1b80, 0x4e8715e5,
	0x1b80, 0x4e8715e7,
	0x1b80, 0x4f1115f5,
	0x1b80, 0x4f1115f7,
	0x1b80, 0x00041605,
	0x1b80, 0x00041607,
	0x1b80, 0xe1ce1615,
	0x1b80, 0xe1ce1617,
	0x1b80, 0xab001625,
	0x1b80, 0xab001627,
	0x1b80, 0x00061635,
	0x1b80, 0x00061637,
	0x1b80, 0x57771645,
	0x1b80, 0x57771647,
	0x1b80, 0x00071655,
	0x1b80, 0x00071657,
	0x1b80, 0x4e861665,
	0x1b80, 0x4e861667,
	0x1b80, 0x00041675,
	0x1b80, 0x00041677,
	0x1b80, 0x00011685,
	0x1b80, 0x00011687,
	0x1b80, 0x00011695,
	0x1b80, 0x00011697,
	0x1b80, 0x7b2416a5,
	0x1b80, 0x7b2416a7,
	0x1b80, 0x7a4016b5,
	0x1b80, 0x7a4016b7,
	0x1b80, 0x790016c5,
	0x1b80, 0x790016c7,
	0x1b80, 0x550316d5,
	0x1b80, 0x550316d7,
	0x1b80, 0x317616e5,
	0x1b80, 0x317616e7,
	0x1b80, 0x7b1c16f5,
	0x1b80, 0x7b1c16f7,
	0x1b80, 0x7a401705,
	0x1b80, 0x7a401707,
	0x1b80, 0x550b1715,
	0x1b80, 0x550b1717,
	0x1b80, 0x31761725,
	0x1b80, 0x31761727,
	0x1b80, 0x7b201735,
	0x1b80, 0x7b201737,
	0x1b80, 0x7a001745,
	0x1b80, 0x7a001747,
	0x1b80, 0x55131755,
	0x1b80, 0x55131757,
	0x1b80, 0x74011765,
	0x1b80, 0x74011767,
	0x1b80, 0x74001775,
	0x1b80, 0x74001777,
	0x1b80, 0x8e001785,
	0x1b80, 0x8e001787,
	0x1b80, 0x00011795,
	0x1b80, 0x00011797,
	0x1b80, 0x570217a5,
	0x1b80, 0x570217a7,
	0x1b80, 0x570017b5,
	0x1b80, 0x570017b7,
	0x1b80, 0x970017c5,
	0x1b80, 0x970017c7,
	0x1b80, 0x000117d5,
	0x1b80, 0x000117d7,
	0x1b80, 0x4f7817e5,
	0x1b80, 0x4f7817e7,
	0x1b80, 0x538817f5,
	0x1b80, 0x538817f7,
	0x1b80, 0xe18c1805,
	0x1b80, 0xe18c1807,
	0x1b80, 0x54801815,
	0x1b80, 0x54801817,
	0x1b80, 0x54001825,
	0x1b80, 0x54001827,
	0x1b80, 0xe18c1835,
	0x1b80, 0xe18c1837,
	0x1b80, 0x54811845,
	0x1b80, 0x54811847,
	0x1b80, 0x54001855,
	0x1b80, 0x54001857,
	0x1b80, 0xe18c1865,
	0x1b80, 0xe18c1867,
	0x1b80, 0x54821875,
	0x1b80, 0x54821877,
	0x1b80, 0x54001885,
	0x1b80, 0x54001887,
	0x1b80, 0xe1971895,
	0x1b80, 0xe1971897,
	0x1b80, 0xbf1d18a5,
	0x1b80, 0xbf1d18a7,
	0x1b80, 0x301d18b5,
	0x1b80, 0x301d18b7,
	0x1b80, 0xe16a18c5,
	0x1b80, 0xe16a18c7,
	0x1b80, 0xe16f18d5,
	0x1b80, 0xe16f18d7,
	0x1b80, 0xe17318e5,
	0x1b80, 0xe17318e7,
	0x1b80, 0xe17a18f5,
	0x1b80, 0xe17a18f7,
	0x1b80, 0xe1e01905,
	0x1b80, 0xe1e01907,
	0x1b80, 0x55131915,
	0x1b80, 0x55131917,
	0x1b80, 0xe1761925,
	0x1b80, 0xe1761927,
	0x1b80, 0x55151935,
	0x1b80, 0x55151937,
	0x1b80, 0xe17a1945,
	0x1b80, 0xe17a1947,
	0x1b80, 0xe1e01955,
	0x1b80, 0xe1e01957,
	0x1b80, 0x00011965,
	0x1b80, 0x00011967,
	0x1b80, 0x54bf1975,
	0x1b80, 0x54bf1977,
	0x1b80, 0x54c01985,
	0x1b80, 0x54c01987,
	0x1b80, 0x54a31995,
	0x1b80, 0x54a31997,
	0x1b80, 0x54c119a5,
	0x1b80, 0x54c119a7,
	0x1b80, 0x54a419b5,
	0x1b80, 0x54a419b7,
	0x1b80, 0x4c1819c5,
	0x1b80, 0x4c1819c7,
	0x1b80, 0xbf0719d5,
	0x1b80, 0xbf0719d7,
	0x1b80, 0x54c219e5,
	0x1b80, 0x54c219e7,
	0x1b80, 0x54a419f5,
	0x1b80, 0x54a419f7,
	0x1b80, 0xbf041a05,
	0x1b80, 0xbf041a07,
	0x1b80, 0x54c11a15,
	0x1b80, 0x54c11a17,
	0x1b80, 0x54a31a25,
	0x1b80, 0x54a31a27,
	0x1b80, 0xbf011a35,
	0x1b80, 0xbf011a37,
	0x1b80, 0xe1ee1a45,
	0x1b80, 0xe1ee1a47,
	0x1b80, 0x54df1a55,
	0x1b80, 0x54df1a57,
	0x1b80, 0x00011a65,
	0x1b80, 0x00011a67,
	0x1b80, 0x54bf1a75,
	0x1b80, 0x54bf1a77,
	0x1b80, 0x54e51a85,
	0x1b80, 0x54e51a87,
	0x1b80, 0x050a1a95,
	0x1b80, 0x050a1a97,
	0x1b80, 0x54df1aa5,
	0x1b80, 0x54df1aa7,
	0x1b80, 0x00011ab5,
	0x1b80, 0x00011ab7,
	0x1b80, 0x7f201ac5,
	0x1b80, 0x7f201ac7,
	0x1b80, 0x7e001ad5,
	0x1b80, 0x7e001ad7,
	0x1b80, 0x7d001ae5,
	0x1b80, 0x7d001ae7,
	0x1b80, 0x55011af5,
	0x1b80, 0x55011af7,
	0x1b80, 0x5c311b05,
	0x1b80, 0x5c311b07,
	0x1b80, 0xe1761b15,
	0x1b80, 0xe1761b17,
	0x1b80, 0xe17a1b25,
	0x1b80, 0xe17a1b27,
	0x1b80, 0x54801b35,
	0x1b80, 0x54801b37,
	0x1b80, 0x54001b45,
	0x1b80, 0x54001b47,
	0x1b80, 0xe1761b55,
	0x1b80, 0xe1761b57,
	0x1b80, 0xe17a1b65,
	0x1b80, 0xe17a1b67,
	0x1b80, 0x54811b75,
	0x1b80, 0x54811b77,
	0x1b80, 0x54001b85,
	0x1b80, 0x54001b87,
	0x1b80, 0xe1761b95,
	0x1b80, 0xe1761b97,
	0x1b80, 0xe17a1ba5,
	0x1b80, 0xe17a1ba7,
	0x1b80, 0x54821bb5,
	0x1b80, 0x54821bb7,
	0x1b80, 0x54001bc5,
	0x1b80, 0x54001bc7,
	0x1b80, 0xe1971bd5,
	0x1b80, 0xe1971bd7,
	0x1b80, 0xbfe91be5,
	0x1b80, 0xbfe91be7,
	0x1b80, 0x301d1bf5,
	0x1b80, 0x301d1bf7,
	0x1b80, 0x00231c05,
	0x1b80, 0x00231c07,
	0x1b80, 0x7b201c15,
	0x1b80, 0x7b201c17,
	0x1b80, 0x7a001c25,
	0x1b80, 0x7a001c27,
	0x1b80, 0x79001c35,
	0x1b80, 0x79001c37,
	0x1b80, 0xe1e41c45,
	0x1b80, 0xe1e41c47,
	0x1b80, 0x00021c55,
	0x1b80, 0x00021c57,
	0x1b80, 0x00011c65,
	0x1b80, 0x00011c67,
	0x1b80, 0x00221c75,
	0x1b80, 0x00221c77,
	0x1b80, 0x7b201c85,
	0x1b80, 0x7b201c87,
	0x1b80, 0x7a001c95,
	0x1b80, 0x7a001c97,
	0x1b80, 0x79001ca5,
	0x1b80, 0x79001ca7,
	0x1b80, 0xe1e41cb5,
	0x1b80, 0xe1e41cb7,
	0x1b80, 0x00021cc5,
	0x1b80, 0x00021cc7,
	0x1b80, 0x00011cd5,
	0x1b80, 0x00011cd7,
	0x1b80, 0x74021ce5,
	0x1b80, 0x74021ce7,
	0x1b80, 0x003f1cf5,
	0x1b80, 0x003f1cf7,
	0x1b80, 0x74001d05,
	0x1b80, 0x74001d07,
	0x1b80, 0x00021d15,
	0x1b80, 0x00021d17,
	0x1b80, 0x00011d25,
	0x1b80, 0x00011d27,
	0x1b80, 0x4d041d35,
	0x1b80, 0x4d041d37,
	0x1b80, 0x2ef81d45,
	0x1b80, 0x2ef81d47,
	0x1b80, 0x00001d55,
	0x1b80, 0x00001d57,
	0x1b80, 0x23301d65,
	0x1b80, 0x23301d67,
	0x1b80, 0x00241d75,
	0x1b80, 0x00241d77,
	0x1b80, 0x23e01d85,
	0x1b80, 0x23e01d87,
	0x1b80, 0x003f1d95,
	0x1b80, 0x003f1d97,
	0x1b80, 0x23fc1da5,
	0x1b80, 0x23fc1da7,
	0x1b80, 0xbfce1db5,
	0x1b80, 0xbfce1db7,
	0x1b80, 0x2ef01dc5,
	0x1b80, 0x2ef01dc7,
	0x1b80, 0x00001dd5,
	0x1b80, 0x00001dd7,
	0x1b80, 0x4d001de5,
	0x1b80, 0x4d001de7,
	0x1b80, 0x00011df5,
	0x1b80, 0x00011df7,
	0x1b80, 0x549f1e05,
	0x1b80, 0x549f1e07,
	0x1b80, 0x54ff1e15,
	0x1b80, 0x54ff1e17,
	0x1b80, 0x54001e25,
	0x1b80, 0x54001e27,
	0x1b80, 0x00011e35,
	0x1b80, 0x00011e37,
	0x1b80, 0x5c311e45,
	0x1b80, 0x5c311e47,
	0x1b80, 0x07141e55,
	0x1b80, 0x07141e57,
	0x1b80, 0x54001e65,
	0x1b80, 0x54001e67,
	0x1b80, 0x5c321e75,
	0x1b80, 0x5c321e77,
	0x1b80, 0x00011e85,
	0x1b80, 0x00011e87,
	0x1b80, 0x5c321e95,
	0x1b80, 0x5c321e97,
	0x1b80, 0x07141ea5,
	0x1b80, 0x07141ea7,
	0x1b80, 0x54001eb5,
	0x1b80, 0x54001eb7,
	0x1b80, 0x5c311ec5,
	0x1b80, 0x5c311ec7,
	0x1b80, 0x00011ed5,
	0x1b80, 0x00011ed7,
	0x1b80, 0x4c981ee5,
	0x1b80, 0x4c981ee7,
	0x1b80, 0x4c181ef5,
	0x1b80, 0x4c181ef7,
	0x1b80, 0x00011f05,
	0x1b80, 0x00011f07,
	0x1b80, 0x5c321f15,
	0x1b80, 0x5c321f17,
	0x1b80, 0x62041f25,
	0x1b80, 0x62041f27,
	0x1b80, 0x63031f35,
	0x1b80, 0x63031f37,
	0x1b80, 0x66071f45,
	0x1b80, 0x66071f47,
	0x1b80, 0x7b201f55,
	0x1b80, 0x7b201f57,
	0x1b80, 0x7a001f65,
	0x1b80, 0x7a001f67,
	0x1b80, 0x79001f75,
	0x1b80, 0x79001f77,
	0x1b80, 0x7f201f85,
	0x1b80, 0x7f201f87,
	0x1b80, 0x7e001f95,
	0x1b80, 0x7e001f97,
	0x1b80, 0x7d001fa5,
	0x1b80, 0x7d001fa7,
	0x1b80, 0x09011fb5,
	0x1b80, 0x09011fb7,
	0x1b80, 0x0c011fc5,
	0x1b80, 0x0c011fc7,
	0x1b80, 0x0ba61fd5,
	0x1b80, 0x0ba61fd7,
	0x1b80, 0x00011fe5,
	0x1b80, 0x00011fe7,
	0x1b80, 0x00000006,
	0x1b80, 0x00000002,
};
#endif

void odm_read_and_config_mp_8195b_cal_init(
	void *dm_void)
{
#if 0
	u32	i = 0;
	u32	array_len = sizeof(array_mp_8195b_cal_init)/sizeof(u32);
	u32	*array = array_mp_8195b_cal_init;

	u32	v1 = 0, v2 = 0;

	PHYDM_DBG(dm, ODM_COMP_INIT, "===> %s\n", __func__);

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];
		odm_config_bb_phy_8195b(dm, v1, MASKDWORD, v2);
		i = i + 2;
	}
#endif
}
