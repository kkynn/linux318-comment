/* 
 * CPC register definitions
 *
 * mips_start_of_header
 *
 * $Id: cpc.h,v 1.1 2009/09/24 04:14:21 chris Exp $
 *
 * mips_end_of_header
 *
 */

// #define CPCBASE_DEFAULT		0x1bde0000
#define CPCBASE_DEFAULT		0x1dde0000 /* For APOLLOPRO- Soc */
#define CPCBASE			CKSEG1ADDR(CPCBASE_DEFAULT)
/* Offsets to major blocks within CPC from CPC base */
#define CPC_GCB_OFS		0x0000 /* Global Control Block */
#define CPC_CLCB_OFS		0x2000 /* Core Local Control Block */
#define CPC_COCB_OFS		0x4000 /* Core Other Control Block */

/* Offsets to individual CPC registers from CPC base */
#define CPCOFS(block,tag,reg) \
	(CPC_##block##_OFS + CPC_##tag##_##reg##_OFS)

#define CPCGCBOFS(reg)		CPCOFS(GCB,GCB,reg)
#define CPCCLCBOFS(reg)		CPCOFS(CLCB,CCB,reg)
#define CPCCOCBOFS(reg)		CPCOFS(COCB,CCB,reg)

/* CPC register access */
#define CPCGCB(reg)		REGP(CPCBASE,CPCGCBOFS(reg))
#define CPCCLCB(reg)		REGP(CPCBASE,CPCCLCBOFS(reg))
#define CPCCOCB(reg)		REGP(CPCBASE,CPCCOCBOFS(reg))

/* GCB registers */
#define CPC_GCB_ACCESS_OFS	0x0000	/* CPC Global Access Privilege */
#define CPC_GCB_SEQDEL_OFS	0x0008	/* CPC Global Sequence Delay Counter */
#define CPC_GCB_RAIL_OFS	0x0010	/* CPC Global Rail Delay Counter */
#define CPC_GCB_RESETLEN_OFS	0x0018	/* CPC Global Reset Width Counter */

/* CPC Core local/Core other control block registers */
#define CPC_CCB_CMD_OFS		0x0000	/* CPC CCB Command */
#define CPC_CCB_STAT_OFS	0x0008	/* CPC CCB Status */
#define CPC_CCB_OTHER_OFS	0x0010	/* CPC CCB Other */

