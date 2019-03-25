/* alan test */

#include <linux/smp.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/gic.h>
#include <asm/r4kcache.h>
#include <asm/mips-cm.h>
#include "bspchip.h"





/**************************************************************/








/*************************************************************/
//For l2cache
// #define _mips_sync() __asm__ __volatile__ ("sync  0x3" : : : "memory")
static int flush_l2cache(void){
	unsigned long cache_size ;
	unsigned long linesz,sets,ways;
	int i;
	unsigned int config2;
	unsigned int tmp;
	config2 = read_c0_config2();
	if (config2 & (1 << 12))
			return 0;
	
			      
                /* L2-Cache */
	tmp = (config2 >> 4) & 0x0f;
	if (0 < tmp && tmp <= 7)
		linesz = 2 << tmp;
	else
		return 0;		
		
	tmp = (config2 >> 8) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		sets = 64 << tmp;
	else
		return 0;

	tmp = (config2 >> 0) & 0x0f;
	if (0 <= tmp && tmp <= 7)
		ways = tmp + 1;
	else
		return 0;
	

	cache_size = sets *  ways * linesz;
	
	printk("L2cache LineSize=%lu, Sets=%lu, Ways=%lu, CacheSize=%lu\n", linesz, sets, ways, cache_size);
	
        for(i=CKSEG0;  i < (CKSEG0 + cache_size); i +=  linesz){
	  cache_op(Index_Writeback_Inv_SD,i);
	}
	__sync_mem();
	return 0;
}







static ssize_t bsp_dw_write_proc(struct file *file, const char __user *buffer,
			 size_t count, loff_t *ppos)
{
   char tmpbuf[32];
   unsigned int addr = 0;
   unsigned int length = 0;
   unsigned type = 0;
   int *ptr = NULL;
   int i = 0;
    /* no data be written */
    if (!count) {
        printk("count is 0\n");
        return 0;
    }

    /* Input size is too large to write our buffer(num) */
    if (count > (sizeof(tmpbuf) - 1)) {
        printk("input is too large\n");
        return -EINVAL;
    }

    if (copy_from_user(tmpbuf, buffer, count)) {
        printk("copy from user failed\n");
        return -EFAULT;
    }
    i = sscanf(tmpbuf, "0x%x 0x%x %d", &addr, &length, &type);
   
    printk("[CPU%d]input addr: %x, length: %x \n", smp_processor_id(),addr, length);
    
    if( (i== 1) && (addr == 1)){
      flush_l2cache();
      return count;
    }
    
    
    if( (addr >> 28) == 0 ){
      return -EINVAL;
    }
    
    
    ptr = (int *)addr;
    if( i == 2 ){
      for(i=0;i<length;i++){
      printk("   %p : 0x%x \n", (ptr+i),*(ptr+i));
      }
    }else
    if( i == 3){
        printk("   %p <= 0x%x \n", (ptr), length);
        *ptr = length;
    }    
    else{
      printk("%p : 0x%x \n", ptr, *ptr);
    }
    
    return count;
}
// 	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
// 	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);

static int bsp_proc_show(struct seq_file *m, void *v)
{
	
	extern unsigned long ebase;
	seq_printf(m, "Cause: 0x%08x\n", read_c0_cause());
	seq_printf(m, "Config: 0x%08x\n", read_c0_config());
	seq_printf(m, "Config1: 0x%08x\n", read_c0_config1());
	seq_printf(m, "Config2: 0x%08x\n", read_c0_config2());
	seq_printf(m, "Config7: 0x%08x\n", read_c0_config7());
	seq_printf(m, "EBASE: 0x%08lx\n", ebase);
	seq_printf(m, "read_gcr_base: 0x%08x\n", read_gcr_base());
	seq_printf(m, "read_gcr_control: 0x%08x\n", read_gcr_control());
	seq_printf(m, "[cpu%d]GCR - Global Config Register : %x ; \n", smp_processor_id(), *(unsigned int *)(KSEG1|(GCMP_BASE_ADDR)) );
	seq_printf(m, "[cpu%d]GIC - Global Config Register : %x ; \n", smp_processor_id(), *(unsigned int *)(KSEG1|(GIC_BASE_ADDR)) );

	return 0;
}

static int bsp_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bsp_proc_show, NULL);
}

static const struct file_operations bsp_dw_proc_fops = {
	.open		= bsp_proc_open,
	.read		= seq_read,
	.write          = bsp_dw_write_proc,
	.llseek		= seq_lseek,
	.release	= single_release,
};



/**************************************************/
static int __init init_modules(void)
{

   proc_create("bsp_dsp", 0444, NULL, &bsp_dw_proc_fops);
   return 0;
}

static void __exit exit_modules(void)
{
    
}

module_init(init_modules);
module_exit(exit_modules);
