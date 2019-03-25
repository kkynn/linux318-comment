/*
 * Copyright (c) 2011, Realtek Semiconductor Corp.
 *
 * file_name: wmpu.c
 *
 *   API of WMPU.
 *
 * Watchpoint and Memory Protection Unit
 *
 * The RX4281/RX5281 is implemented with watchpoint, another supplementary feature is
 * memory protection.
 *
 * Jethro Hsu (jethro@realtek.com)
 * Jul. 15, 2011
 */

#include <linux/slab.h>

#include <asm/wmpu.h>

callback wmpu_handler;
/*
 * rlx_wmpulohi_cfg_alloc():
 * Allocate a descriptor of WATCHLO and WATCHHI with initial value.
 *
 */
struct wmpu_lohi_cfg * rlx_wmpulohi_cfg_alloc(void)
{
	struct wmpu_lohi_cfg *cfg = (struct wmpu_lohi_cfg *) kmalloc(sizeof(struct wmpu_lohi_cfg),GFP_ATOMIC);
		 cfg->cfg_num = 0x0;
		 cfg->wmpulo_addr = 0x0;
		 cfg->wmpulo_attr = 0x0;
		 cfg->wmpuhi_mask = 0x0;
		 cfg->wmpuhi_asid = 0x0;
		 cfg->wmpuhi_asid_en = 0x0;
	return cfg;
}

/*
 * rlx_wmpuctl_cfg_alloc():
 * Allocate a descriptor of WMPCTL with initial value.
 *
 */

struct wmpu_ctl_cfg * rlx_wmpuctl_cfg_alloc(void)
{
        struct wmpu_ctl_cfg *cfg = (struct wmpu_ctl_cfg *) kmalloc(sizeof(struct wmpu_ctl_cfg),GFP_ATOMIC);
                 cfg->entry_en = 0x0;
                 cfg->kernel_en = 0x0;
                 cfg->mode = 0x0;
        return cfg;
}

/*
 * rlx_set_wmpu_reg():
 * There are four registers, reg0~reg3, used by ptrace for watch function.
 * Set values to the other four registers according to descriptor .
 *
 */

void rlx_set_wmpu_reg(struct wmpu_lohi_cfg *cfg)
{
	unsigned int wmpuhi,maskhi;
	wmpuhi = (cfg->wmpuhi_mask & 0xff8) | (cfg->wmpuhi_asid << WMPHI_ASID) | \
		 ((!(cfg->wmpuhi_asid_en & 0x1)) << WMPHI_ASID_EN);
	maskhi =  cfg->wmpuhi_mask & 0xfffff000;

	switch (cfg->cfg_num) {
	case 4:
		write_c0_watchlo4((cfg->wmpulo_addr & 0xfffffff8) | cfg->wmpulo_attr);
		write_c0_watchhi4(wmpuhi);
		write_lxc0_wmpxmask4(maskhi);
		break;
	case 5:
                write_c0_watchlo5((cfg->wmpulo_addr & 0xfffffff8) | cfg->wmpulo_attr);
		write_c0_watchhi5(wmpuhi);
                write_lxc0_wmpxmask5(maskhi);
                break;
	case 6:
                write_c0_watchlo6((cfg->wmpulo_addr & 0xfffffff8) | cfg->wmpulo_attr);
		write_c0_watchhi6(wmpuhi);
                write_lxc0_wmpxmask6(maskhi);
                break;
	case 7:
                write_c0_watchlo7((cfg->wmpulo_addr & 0xfffffff8) | cfg->wmpulo_attr);
		write_c0_watchhi7(wmpuhi);
                write_lxc0_wmpxmask7(maskhi);
                break;
	default:
                BUG();
		break;
	}

}

/*
 * rlx_set_wmpuctl_reg():
 * Set values to WMPUCTL according to descriptor.
 */

void rlx_set_wmpuctl_reg(struct wmpu_ctl_cfg *cfg)
{
	unsigned char entry_en, kernel_en, mode;
	unsigned wmpctl;
	entry_en = cfg->entry_en;
	kernel_en = cfg->kernel_en;
	mode = cfg->mode;
	rlxi_wmpu_ctl_free(cfg);
	wmpctl = read_lxc0_wmpctl();
	wmpctl = wmpctl | ((entry_en << WMPCTL_EE) | (kernel_en << WMPCTL_KE) | mode);
	write_lxc0_wmpctl(wmpctl);
}


/*
 * rlx_wmpu_info():
 * Allocate an descriptor for WMPUSTATUS and WMPVADDR, then read current status from them.
 */

struct wmpu_info * rlx_wmpu_info(void)
{
	struct wmpu_info *info = (struct wmpu_info *)kmalloc(sizeof(struct wmpu_info),GFP_ATOMIC);
	info->wmpuvaddr = read_lxc0_wmpvaddr();
	info->entry_match = (read_lxc0_wmpstatus() & ENTRY_MATCH) >> 16;
	info->attr_match = (read_lxc0_wmpstatus() & 0x7);
	return info;
}

/*
 *   Deallocators
 */

void rlx_wmpulohi_cfg_free(struct wmpu_lohi_cfg *cfg)
{
	kfree(cfg);
}

void rlxi_wmpu_ctl_free(struct wmpu_ctl_cfg *cfg)
{
	kfree(cfg);
}

void rlx_wmpu_info_free(struct wmpu_info *info)
{
	kfree(info);
}

/*
 * rlx_set_wmpu_addr(): Set the address and range to WATCHLO and WATCHHI.
 * Parameters:
 * addr: An address will be set to VADDR of WATCHLO register.
 * mask: The bit in mask is 1 inhibits the corresponding address bit in WATCHLO from paticipating in
 *       the address match.
 * attr: The last thress bits, I, R, and W in WATCHLO represent different memory access type:
 *       instruction execution, data read, and data write.
 * reg_num: A decimal value to declare which register is selected.
 * asid: ASID is a 6-bit address ID matching field, need address matching when ASID is the same with
 *       ASID field in ENTRYHI.
 * asid_en: An asid control bit. If the bit is 1, address matching will ignore ASID field.
 *
 */

int rlx_set_wmpu_addr(unsigned int addr, unsigned int mask, unsigned char attr, int reg_num, unsigned char asid, unsigned char asid_en)
{
	struct wmpu_lohi_cfg *cfg = rlx_wmpulohi_cfg_alloc();
	if(reg_num > 7 || reg_num < 4)
	{
		printk(KERN_INFO "Only 4 wmpu registers, No.4~No7, provided. Please reassign your register number\n");
		rlx_wmpulohi_cfg_free(cfg);
		return WMPU_FAIL;
	}

	cfg->cfg_num = reg_num;
	cfg->wmpulo_addr = addr;
	cfg->wmpulo_attr = attr;
	cfg->wmpuhi_mask = mask;
	cfg->wmpuhi_asid = asid;
	cfg->wmpuhi_asid_en = asid_en;

	rlx_set_wmpu_reg(cfg);
	rlx_wmpulohi_cfg_free(cfg);
	return WMPU_PASS;
}

/*
 * rlx_set_wmpu_addr(): Setup WMPCTL.
 * Parameters:
 * entry_enable: 1 for enable dedicated entry, 0 for disable.
 * kernel_enable: 0 for no memory protection or watchpoint under kernel mode, 1 for enable.
 * mode: Switch mode between watchpoint(0) and memory protection(1).
 */

int rlx_set_wmpu_ctl(unsigned char entry_enable, unsigned char kernel_enable, unsigned char mode)
{
	struct wmpu_ctl_cfg *cfg = rlx_wmpuctl_cfg_alloc();
	if(entry_enable & 0x0f)
		return WMPU_FAIL;
	cfg->entry_en = entry_enable;
	cfg->kernel_en = kernel_enable & 0x1;
	cfg->mode = mode & 0x1;
	rlx_set_wmpuctl_reg(cfg);
	return WMPU_PASS;
}

/*
 * rlx_get_wmpu_info(): Get content of WMPUSTATUS and WMPUVADDR when Watchpoint or Memory Protection
 *                       exception happens.
 * Parameters:
 * addr: Virtual address that cause the watch exception.
 * entry_match: Then coresponding bit indicate which entry triggering exception.
 * attr_match: The coresponding bit indicate the access type triggering exception.
 *
*/

void rlx_get_wmpu_info(unsigned int *addr, unsigned char *entry_match, unsigned char *attr_match)
{
	struct wmpu_info *info = rlx_wmpu_info();
	*addr = info->wmpuvaddr;
	*entry_match = info->entry_match;
	*attr_match = info->attr_match;
	rlx_wmpu_info_free(info);
}

/*
 * rlx_wmpu_reset():
 * Initialize and turn off WMPU.
 */

void rlx_wmpu_reset(void)
{
	unsigned int wmpctl;
        write_c0_watchlo4(0);
        write_c0_watchhi4(0);
        write_lxc0_wmpxmask4(0);
        write_c0_watchlo5(0);
        write_c0_watchhi5(0);
        write_lxc0_wmpxmask5(0);
        write_c0_watchlo6(0);
        write_c0_watchhi6(0);
        write_lxc0_wmpxmask6(0);
        write_c0_watchlo7(0);
        write_c0_watchhi7(0);
        write_lxc0_wmpxmask7(0);
	wmpctl = read_lxc0_wmpctl();
	write_lxc0_wmpctl(wmpctl & 0xff0f0000);
}

/*
 * rlx_install_wmpu_handler():
 * Install a WMPU exception handler.
 * The format of handler as following:
 * "void (* callback) (unsigned int addr, unsigned char entry, unsigned char attr)"
 */

void rlx_install_wmpu_handler (callback handler)
{
	wmpu_handler = handler;
}
