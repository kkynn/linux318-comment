#include <linux/kernel.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>

#include <nor_spi/nor_spif_core.h>

#if !defined(CONFIG_DEFAULTS_KERNEL_3_18)
	#define CONFIG_DEFAULTS_KERNEL_3_18 0
#endif

#if !defined(CONFIG_KERNEL_2_6_30)
	#define CONFIG_KERNEL_2_6_30 0
#endif

#if (CONFIG_DEFAULTS_KERNEL_3_18 == 1)
	#include <linux/module.h>
	#include <linux/semaphore.h>
	#include <linux/slab.h>
	#define ERASE_P _erase
	#define READ_P _read
	#define WRITE_P _write
	#define UNLOCK_P _unlock
	#define LOCK_P _lock
	#define SUSPEND_P _suspend
	#define RESUME_P _resume
	#define SYNC_P _sync
	#define POINT_P _point
	#define UNPOINT_P _unpoint
	DEFINE_SEMAPHORE(snof_mutex);
	#define SNOF_CNTLR_LOCK_INIT() do { sema_init(&snof_mutex, 1); } while (0)
#elif (CONFIG_KERNEL_2_6_30 == 1)
	#define ERASE_P erase
	#define READ_P read
	#define WRITE_P write
	#define UNLOCK_P unlock
	#define LOCK_P lock
	#define SUSPEND_P suspend
	#define RESUME_P resume
	#define SYNC_P sync
	#define POINT_P point
	#define UNPOINT_P unpoint
	DECLARE_MUTEX(snof_mutex);
	#define SNOF_CNTLR_LOCK_INIT() do { } while (0)
#else
	#error EE: Unknown kernel version
#endif

#define SNOF_CNTLR_LOCK() do {									\
		down(&snof_mutex); \
	} while (0)
#define SNOF_CNTLR_UNLOCK() do { \
		up(&snof_mutex); \
	} while (0)

extern void dump_stack(void);

extern norsf_g2_info_t norsf_info;

static int luna_snof_mtd_erase(struct mtd_info *mtd, struct erase_info *instr) {
	unsigned long ofs, len;
	int ret;

	ofs = instr->addr;
	len = instr->len;

	SNOF_CNTLR_LOCK();
	ret = NORSF_ERASE(ofs, len, 0, 1);
	SNOF_CNTLR_UNLOCK();

	if (ret)
		return -EIO;

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int luna_snof_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
                              size_t *retlen, u_char *buf) {
	unsigned long ofs;
	int ret;

	ofs = from;
	*retlen = 0;

	SNOF_CNTLR_LOCK();
	ret = NORSF_READ(ofs, len, buf, 0);
	SNOF_CNTLR_UNLOCK();

	if (ret)
		return -EIO;

	if (retlen)
		*retlen = len;

	return ret;
}

static int luna_snof_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
                               size_t *retlen, const u_char *buf) {
	unsigned long ofs;
	int ret;

	ofs = to;
	*retlen = 0;

	SNOF_CNTLR_LOCK();
	ret = NORSF_PROG(ofs, len, buf, 0);
	SNOF_CNTLR_UNLOCK();

	if (ret)
		return -EIO;

	if (retlen)
		*retlen = len;

	return ret;
}

/* Luna SNOF g3 driver has no lock/unlock issue.
	 This function is called indirectly from sys_ioctl() so it is implemented to reponse ok */
static int luna_snof_mtd_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len) {
	return 0;
}

static int luna_snof_erasesize(const norsf_g2_info_t *ni) {
	const norsf_erase_cmd_t *cmds = ni->cmd_info->cerase;
	int i = 0;
	int erasesize = 4096;

	for (i=0; i<ni->cmd_info->cerase_cmd_num; i++) {
		if (cmds[i].offset_lmt == 0) {
			erasesize = cmds[i].sz_b;
		}
	}
	return erasesize;
}

#define MTD_DUMMY_FUNC(func_name) \
	static int luna_snof_mtd_##func_name(void) { \
		printk(KERN_WARNING "WW: %s: Invoked\n", \
					 __FUNCTION__); \
		dump_stack(); \
		return 0; \
	}
MTD_DUMMY_FUNC(lock);
MTD_DUMMY_FUNC(suspend);
MTD_DUMMY_FUNC(resume);
MTD_DUMMY_FUNC(destory);

#define LUNA_SNOF_G3_NULL_FUNC NULL

int luna_snof_g3_remap_1fc00000(void) {
#define MCR (0xb8001000)
#define ROMSAR (0xb8004080)
#define ROMSSR (0xb8004084)
#define O0BTCR (0xb8005100)
#define O0BTMAR (0xb8005108)
	uint32_t reg_value;
	uint32_t o0btcr_backup;

	printk("II: Redirecting 0x1fc00000...\n");

	/* Check if ROM is enabled */
	reg_value = REG32(ROMSAR);
	if (reg_value) {
		printk("EE: ROM is already enabled\n");
		return -1;
	}

	/* Temp. enable OC0 bus timeout monitor */
	o0btcr_backup = REG32(O0BTCR);
	REG32(O0BTCR) = 0x9c000000;

	/* Clean up pending reads to 0x1fc00000 */
	reg_value = REG32(0xbfc00000);

	/* Disable flash mapping */
	reg_value = REG32(MCR);
	REG32(MCR) = reg_value | (1 << 19);

	/* Enable ROM mapping */
	REG32(ROMSSR) = 0x00000008; //32KB
	REG32(ROMSAR) = 0x1fc00001; //Map ROM to 1fc00000

	if (REG32(O0BTMAR) == 0x1fc00000) {
		printk("WW: 0x1fc00000 is triggered");
	}

	/* Restore OC0 BTM */
	REG32(O0BTCR) = o0btcr_backup;

	printk("II: Redirection done\n");

	return 0;
}


struct mtd_info *luna_snof_g3_probe(struct map_info *map) {
	struct mtd_info *mtd = NULL;

	SNOF_CNTLR_LOCK_INIT();

	if (luna_snof_g3_remap_1fc00000()) {
		printk(KERN_ERR "Failed to remap 0x1fc00000\n");
		return NULL;
	}

	printk("Luna SPI NOR FLASH G3 driver-");
	norsf_detect();

	mtd = kzalloc(sizeof(*mtd), GFP_KERNEL);
	if (!mtd) {
		printk(KERN_ERR "Failed to allocate memory for MTD device\n");
		return NULL;
	}
	mtd->priv = map;
	mtd->type = MTD_NORFLASH;
	mtd->flags = MTD_CAP_NORFLASH;
	mtd->name = map->name;
	mtd->writesize = 1;

	mtd->ERASE_P = luna_snof_mtd_erase;
	mtd->READ_P = luna_snof_mtd_read;
	mtd->WRITE_P = luna_snof_mtd_write;
	mtd->UNLOCK_P = luna_snof_mtd_unlock;
	mtd->LOCK_P = (typeof(mtd->LOCK_P))luna_snof_mtd_lock;
	mtd->SUSPEND_P = (typeof(mtd->SUSPEND_P))luna_snof_mtd_suspend;
	mtd->RESUME_P = (typeof(mtd->RESUME_P))luna_snof_mtd_resume;

	/* Luna SNOF g3 driver needs no sync. Things after MTD wrapper are all atomic. */
	mtd->SYNC_P = LUNA_SNOF_G3_NULL_FUNC;

	/* Luna SNOF g3 driver needs no point/unpoint. MMIO read is not allowed outside driver. */
	mtd->POINT_P = LUNA_SNOF_G3_NULL_FUNC;
	mtd->UNPOINT_P = LUNA_SNOF_G3_NULL_FUNC;

	mtd->erasesize = luna_snof_erasesize(&norsf_info);
	if (mtd->erasesize != 4096) {
		printk(KERN_WARNING "\nWW: %s: Erase size is %d KB, make sure flash layout matched!\n",
		       __FUNCTION__,
		       mtd->erasesize / 1024);
	}
	mtd->numeraseregions = 0;

	mtd->size = (norsf_info.size_per_chip_b * norsf_info.num_chips);

	if (mtd->size > map->size) {
		printk(KERN_WARNING "WW: %s: Reducing visibility of %ldKiB chip to %ldKiB\n",
		       __FUNCTION__,
		       (unsigned long)mtd->size >> 10,
		       (unsigned long)map->size >> 10);
		mtd->size = map->size;
	}
	return mtd;
}

struct mtd_chip_driver luna_snof_g3_chipdrv = {
	.probe = luna_snof_g3_probe,
	.destroy = (typeof(luna_snof_g3_chipdrv.destroy))luna_snof_mtd_destory,
	.name = "map_rom",
	.module = THIS_MODULE
};

int __init luna_snof_init(void) {
	register_mtd_chip_driver(&luna_snof_g3_chipdrv);
	return 0;
}

void __exit luna_snof_exit(void) {
	unregister_mtd_chip_driver(&luna_snof_g3_chipdrv);
	return;
}

module_init(luna_snof_init);
module_exit(luna_snof_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Realtek Semiconductor Corp.");
MODULE_DESCRIPTION("NOR SPI-F Cntlr. Driver");
