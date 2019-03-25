#ifndef RTL_ATM_H
#define RTL_ATM_H

/*--- vendor-specific for atmdev.h ---*/
#define ATM_SETBRIDGEPPPOE _IOW('a',ATMIOC_SPECIAL+6,atm_backend_t)
#define ATM_SETVLAN     _IOW('a',ATMIOC_SPECIAL+7,atm_backend_t)
                                        /* set VLAN mapping */
#define ATM_SETITFGRP   _IOW('a',ATMIOC_SPECIAL+8,atm_backend_t)
                                        /* set interface group */
#define ATM_SETFGROUP _IOW('a',ATMIOC_SPECIAL+9,atm_backend_t)

/*--- vendor-specific for atmbr2684.h ---*/
struct vlan {
	int	vlan;		/* vlan flag */
	int	vid;		/* vlan tag */
	int	vlan_prio;	/* vlan priority bits */
	int	vlan_pass;	/* vlan passthrough */
};

struct itfgrp {
	int flag;		/* group flag */
	int member;		/* bit-mapped LAN interface member */
};

/*
 * This is for the vendor-specific ATM_SETBACKEND call - these are like socket families:
 */
struct atm_backend_vendor {
	atm_backend_t backend_num;	/* ATM_BACKEND_BR2684 */
	struct br2684_if_spec ifspec;
	struct vlan vlan_tag;
	struct itfgrp ifgrp;	/* interface group */
	uint16_t fgroup;	/* portmapping */
};

#endif

