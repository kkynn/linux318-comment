/*
 * Copyright (C) 2011 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 *
 * Purpose : Switch NBLOn+LUBOn
 * The NBLOn/NBLOff bits controls the non-blocking load function. 
 * The LUBOn/LUBOff bits controls the loadused buffer function.
 *
 * Feature : 
 *# echo 1 >  /proc/luna_nbl/nbl_switch 
 * write value 0x00000001 
 * Enable CPU's NBL.CCTL=0x14000000 
 *# echo 0 >  /proc/luna_nbl/nbl_switch 
 * write value 0x00000000  
 * Disable CPU's NBL.CCTL=0x28000000
 */

 
/*
  * Include Files
  */
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/mipsregs.h>

#include <bspchip.h>
#include <bspchip_8686.h>

static int nbl_flag = 0;
/* enable nob-blocking load , shoud flush 
 * all instruction from pipeline 
 */
static inline void nbl_hazard(void)
{
	__asm__ __volatile__(
	  	".set	push	\n\t"
		".set noreorder\n\t"
		"nop\n\t"
		"sync\n\t"
		"nop\n\t"
		".set pop");
}

struct proc_dir_entry *nbl_proc_dir = NULL;


static void nbl_switch(int flag){
  unsigned long flags = 0;
  switch (flag){
    case 1:
      local_irq_save(flags);
      write_c0_cctl0(0x00000000);
      nbl_hazard();
      write_c0_cctl0(0x14000000);
      nbl_hazard();
      nbl_flag = 1;
      printk("Enable CPU's NBL.CCTL=0x%08x\n", read_c0_cctl0());
      local_irq_restore(flags);
      break;
    case 0:
      local_irq_save(flags);
      write_c0_cctl0(0x00000000);
      nbl_hazard();
      write_c0_cctl0(0x28000000);
      nbl_hazard();
      nbl_flag = 0;
      printk("Disable CPU's NBL.CCTL=0x%08x\n", read_c0_cctl0());
      local_irq_restore(flags);
      break;    
    default:
      printk("Only accept 1/0 to enable/disable. CCTL=0x%08x\n", read_c0_cctl0());
  } 
}


static int nbl_switch_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, "[CCTL0 = 0x%08x] CPU's Non-Blocking Load : %s\n", read_c0_cctl0(), (nbl_flag == 1)?"Enable":"Disable");
    return 0;
}

static ssize_t nbl_switch_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int inp_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        inp_val = simple_strtoul(tmpBuf, NULL, 0);
        printk("write value 0x%08x\n", inp_val);
	nbl_switch(inp_val);		
        return size;
    }
    return -EFAULT;
}



static int nbl_switch_open(struct inode *inode, struct file *file)
{
    return single_open(file, nbl_switch_show, inode->i_private);
}
static const struct file_operations bsp_luna_nbl_fops = {
    .owner      = THIS_MODULE,
    .open       = nbl_switch_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = nbl_switch_write,
};


int __init luna_nbl_switch_init(void)
{
    nbl_proc_dir = proc_mkdir("luna_nbl", NULL);
    if (nbl_proc_dir == NULL) {
        printk("create /proc/luna_nbl failed!\n");
        return 1;
    }
    
    if(! proc_create("nbl_switch", 0444, nbl_proc_dir, &bsp_luna_nbl_fops)){
      printk("create proc luna_nbl/nbl_switch failed!\n");
      return 1;
    }
    
    return 0;
}

void __exit luna_nbl_switch_exit(void)
{
     remove_proc_entry("nbl_switch", nbl_proc_dir);
     proc_remove(nbl_proc_dir);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek Taroko CPU : NBL Switch module");
module_init(luna_nbl_switch_init);
module_exit(luna_nbl_switch_exit);
