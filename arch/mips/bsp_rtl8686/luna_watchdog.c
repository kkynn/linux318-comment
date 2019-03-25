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
 * Purpose : watchdog timeout kernel thread
 *
 * Feature : Use kernel thread to perodically kick the watchdog
 *
 */

 
/*
  * Include Files
  */
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/init.h> 
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <bspchip.h>
#include <bspchip_8686.h>

/*
 * Data Declaration
 */
unsigned int watchdog_flag = 0;
unsigned int kick_wdg_time = 5;

unsigned int wdg_timeout = CONFIG_WDT_PH1_TO;
unsigned int wdg_timeout_ph2 = CONFIG_WDT_PH2_TO;
unsigned int wdg_clk_sc = CONFIG_WDT_CLK_SC;
unsigned int wdg_rest_mode = CONFIG_WDT_RESET_MODE;
struct proc_dir_entry *watchdog_proc_dir = NULL;
#if 0
struct proc_dir_entry *watchdog_flag_entry = NULL;
struct proc_dir_entry *wdg_timeout_entry = NULL;
struct proc_dir_entry *kick_wdg_time_entry = NULL;
struct proc_dir_entry *wdg_register_entry = NULL;
#endif
#define DEBUG_LOCK_TEST
#ifdef DEBUG_LOCK_TEST
struct proc_dir_entry *wdt_lock_entry = NULL;
DEFINE_SPINLOCK(wdt_lock);
#endif

struct task_struct *pWatchdogTask;

#define RTK_WDG_EN()   (REG32(BSP_WDTCTRLR) |= WDT_E)
#define RTK_WDG_DIS()  (REG32(BSP_WDTCTRLR) &= (~WDT_E))
#define RTK_WDG_KICK() (REG32(BSP_WDCNTRR) = REG32(BSP_WDCNTRR)| WDT_KICK )

#define _DEBUG_RTK_WDT_ALWAYS_   0xffffffff

#define DEBUG_RTK_WDT
#ifdef DEBUG_RTK_WDT
static unsigned int _debug_rtk_wdt_ = (_DEBUG_RTK_WDT_ALWAYS_);//|DEBUG_READ|DEBUG_WRITE|DEBUG_ERASE);
static int debug_mask = 0;
#define DEBUG_RTK_WDT_PRINT(mask, string) \
            if ((_debug_rtk_wdt_ & mask) || (mask == _DEBUG_RTK_WDT_ALWAYS_)) \
            printk string
#else
#define DEBUG_RTK_WDT_PRINT(mask, string)
#endif


void luna_watchdog_kick(void) {
	RTK_WDG_KICK();
}

struct timeval start;
struct timeval end;

static void show_usage(void)
{
    unsigned int reg_val = REG32(BSP_WDTCTRLR); 
    printk("************ Watchdog Setting ****************\n");
    printk("WDT_E=%d, (1-enable, 0-disable)\n", ( (reg_val >> 31) & 0x1));
    printk("LX(MHz)=%u\n", BSP_MHZ);
    printk("WDT_CLK_SC=%d\n", ( (reg_val >> 29) & 0x3));
    printk("PH1_TO=%d\n", ( (reg_val >> 22) & 0x1F));
    printk("PH2_TO=%d\n", ( (reg_val >> 15) & 0x1F));
    printk("WDT_RESET_MODE=%d\n", ( (reg_val ) & 0x3));
    printk("**********************************************\n");
}

//for LX 200M Hz
void set_rtk_wdt_ph1_threshold(int sec)
{
    unsigned int reg=0;

    reg = REG32(BSP_WDTCTRLR);
    reg &= ~(WDT_PH12_TO_MSK << WDT_PH1_TO_SHIFT);
    
    reg &= ~(WDT_PH12_TO_MSK << WDT_PH2_TO_SHIFT);
    reg |= (wdg_timeout_ph2 << WDT_PH2_TO_SHIFT);
    
    reg |= (wdg_clk_sc << WDT_CLK_SC_SHIFT);
    reg |= wdg_rest_mode;
    
    if(sec ==0){
            REG32(BSP_WDTCTRLR) = reg | (31 << WDT_PH1_TO_SHIFT);
    }else{
            REG32(BSP_WDTCTRLR) = reg | (sec << WDT_PH1_TO_SHIFT);
    }
}



void kick_rtk_watchdog(void)
{
    if (_debug_rtk_wdt_ & debug_mask){
    do_gettimeofday(&end);
        printk("tv_sec:%u\t",(unsigned int)end.tv_sec);
        printk("tv_usec:%u\n",(unsigned int)end.tv_usec);
    }
    RTK_WDG_KICK();
}

int watchdog_timeout_thread(void *data)
{


    while(!kthread_should_stop())
    {
        //gettimeofday(&start,NULL);
        /* No need to wake up earlier */
        set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_timeout(kick_wdg_time * HZ);    
        if(watchdog_flag){
            kick_rtk_watchdog();
        }
    }
    return 0;
}

static void wdt_thread_maintain(int flag){

    if(flag){
            if( pWatchdogTask == NULL){
            pWatchdogTask = kthread_create(watchdog_timeout_thread, NULL, "luna_watchdog");
            if(IS_ERR(pWatchdogTask))
            {
                printk("[Kthread : luna_watchdog] init failed %ld!\n", PTR_ERR(pWatchdogTask));
            }
            else
            {
                wake_up_process(pWatchdogTask);
                printk("[Kthread : luan_watchdog ] init complete!\n");
            }
            }
    }else{
        if( pWatchdogTask != NULL){
            kthread_stop(pWatchdogTask);
            pWatchdogTask = NULL;
        }       
    }

}

/* Kthread watchdog */
static int kick_wdg_time_show(struct seq_file *seq, void *v)
{
      seq_printf(seq, "[kick_wdg_time = %d sec]\n", kick_wdg_time);
      seq_printf(seq, "set watchdog kick time to 1~31 sec\n");
      return 0; 
}


static ssize_t  kick_wdg_time_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;

    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        kick_wdg_time = simple_strtoul(tmpBuf, NULL, 10);
        printk("write kick_wdg_time to %d\n", kick_wdg_time);
        //set_wdt_ph1_threshold(wdg_timeout);
        return size;
    }
    return -EFAULT;
}

/* ƒÜ    Watch Dog Timer Control Register: PH1 */
static int wdg_timeout_show(struct seq_file *seq, void *v)
{
    unsigned int reg_val;   
    reg_val = REG32(BSP_WDTCTRLR);
    seq_printf(seq, "WDT_E=%d, (1-enable, 0-disable)\n", ( (reg_val >> 31) & 0x1));
    seq_printf(seq, "LX(MHz)=%u\n", BSP_MHZ);
    seq_printf(seq, "WDT_CLK_SC=%d\n", ( (reg_val >> 29) & 0x3));
    seq_printf(seq, "PH1_TO=%d\n", ( (reg_val >> 22) & 0x1F));
    seq_printf(seq, "PH2_TO=%d\n", ( (reg_val >> 15) & 0x1F));
    seq_printf(seq, "WDT_RESET_MODE=%d\n", ( (reg_val ) & 0x3));
    return 0;
}

static ssize_t wdg_timeout_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;

    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        wdg_timeout = simple_strtoul(tmpBuf, NULL, 10);
        printk("write wdg_timeout to value(%d)\n", wdg_timeout);
        set_rtk_wdt_ph1_threshold(wdg_timeout);
        return size;
    }
    return -EFAULT;
}


static int watchdog_debug_show(struct seq_file *seq, void *v)
{


    seq_printf(seq, "[watchdog_flag = 0x%08x]\n", watchdog_flag);
    seq_printf(seq, "0x1: enable hw watchdog timeout\n");
    seq_printf(seq,"0x3: enable hw watchdog timeout debug message\n");


    return 0;
}

static ssize_t watchdog_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;

    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        watchdog_flag = simple_strtoul(tmpBuf, NULL, 16);
        printk("write watchdog_flag to 0x%08x\n", watchdog_flag);
        if(watchdog_flag){
            wdt_thread_maintain(1);
            set_rtk_wdt_ph1_threshold(wdg_timeout);
            RTK_WDG_EN();
            RTK_WDG_KICK();
        }else{
            RTK_WDG_DIS();
            wdt_thread_maintain(0); //stop thread
        }
        show_usage();
        if(watchdog_flag & 0x02){
            debug_mask = 1;
        }else{
                debug_mask = 0;
        }
            
        return size;
    }
    return -EFAULT;
}


static ssize_t proc_wdt_registers_r(struct seq_file *seq, void *v)
{   

    int i;

    for(i=0; i<3; i++) {
        seq_printf(seq, "REG32(0x%08x)=0x%08x\n", (0xb8003260+(i*4)), REG32((0xb8003260+(i*4))));
    }

    return 0;
}

#ifdef DEBUG_LOCK_TEST

static ssize_t wdt_lock_read(struct seq_file *seq, void *v)
{


    seq_printf(seq, "[watchdog_flag = 0x%08x]\n", watchdog_flag);

    return 0;
}
static void wdt_lock_func(void){
    spin_lock(&wdt_lock);
    printk("[Watchdog Timeout Testing]Dead lock forever.");
    while(1);
        
}

static ssize_t wdt_lock_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int lock_flag = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        lock_flag = simple_strtoul(tmpBuf, NULL, 10);
        printk("write lock_flag to 0x%08x\n", lock_flag);
        if(lock_flag){
            wdt_lock_func();
        }
        return size;
    }
    return -EFAULT;
}

#endif
static int watchdog_open(struct inode *inode, struct file *file)
{
    return single_open(file, watchdog_debug_show, inode->i_private);
}
static const struct file_operations bsp_luna_wdt_flag_fops = {
    .owner      = THIS_MODULE,
    .open       = watchdog_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = watchdog_write,
};


static int timeout_open(struct inode *inode, struct file *file)
{
    return single_open(file, wdg_timeout_show, inode->i_private);
}

static const struct file_operations bsp_luna_wdt_timeout_fops = {
    .owner      = THIS_MODULE,
    .open       = timeout_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = wdg_timeout_write,
};


static int kick_open(struct inode *inode, struct file *file)
{
    return single_open(file, kick_wdg_time_show, inode->i_private);
}

static const struct file_operations bsp_luna_wdt_kick_fops = {
    .owner      = THIS_MODULE,
    .open       = kick_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = kick_wdg_time_write,
};



static int regs_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_wdt_registers_r, inode->i_private);
}

static const struct file_operations bsp_luna_wdt_regs_fops = {
    .owner      = THIS_MODULE,
    .open       = regs_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};


#ifdef DEBUG_LOCK_TEST
static int locktest_open(struct inode *inode, struct file *file)
{
    return single_open(file, wdt_lock_read, inode->i_private);
}
static const struct file_operations bsp_luna_wdt_locktest_fops = {
    .owner      = THIS_MODULE,
    .open       = locktest_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = wdt_lock_write,
};

#endif

static void watchdog_dbg_init(void)
{
    /* Create proc debug commands */
    watchdog_proc_dir = proc_mkdir("luna_watchdog", NULL);
    if (watchdog_proc_dir == NULL) {
        printk("create /proc/luna_watchdog failed!\n");
        return;
    }
    
#if 0
        watchdog_flag_entry = create_proc_entry("watchdog_flag", 0, watchdog_proc_dir);
        if(watchdog_flag_entry){
            watchdog_flag_entry->read_proc = watchdog_read;
            watchdog_flag_entry->write_proc = watchdog_write;
        }
        wdg_timeout_entry = create_proc_entry("wdg_timeout", 0, watchdog_proc_dir);
        if(wdg_timeout_entry){
            wdg_timeout_entry->read_proc = wdg_timeout_read;
            wdg_timeout_entry->write_proc = wdg_timeout_write;
        }
        kick_wdg_time_entry = create_proc_entry("kick_wdg_time", 0, watchdog_proc_dir);
        if(kick_wdg_time_entry){
            kick_wdg_time_entry->read_proc = kick_wdg_time_read;
            kick_wdg_time_entry->write_proc = kick_wdg_time_write;
        }
        
        wdg_register_entry = create_proc_entry("registers", 0, watchdog_proc_dir);
        if(wdg_register_entry){
            wdg_register_entry->read_proc = proc_wdt_registers_r;
        }
        
        
#ifdef DEBUG_LOCK_TEST
        wdt_lock_entry = create_proc_entry("wdt_lock_test", 0, watchdog_proc_dir);
        if(wdt_lock_entry){
            wdt_lock_entry->read_proc = wdt_lock_read;
            wdt_lock_entry->write_proc = wdt_lock_write;
        }
#endif
#endif
    if(! proc_create("watchdog_flag", 0444, watchdog_proc_dir, &bsp_luna_wdt_flag_fops)){
      printk("create proc wdt/watchdog_flag failed!\n");
      return;
    }
 
    if(! proc_create("wdg_timeout", 0444, watchdog_proc_dir, &bsp_luna_wdt_timeout_fops)){
      printk("create proc wdt/wdg_timeout failed!\n");
      return;
    }
 
    if(! proc_create("kick_wdg_time", 0444, watchdog_proc_dir, &bsp_luna_wdt_kick_fops)){
      printk("create proc wdt/kick_wdg_time failed!\n");
      return;
    }
 
     if(! proc_create("registers", 0444, watchdog_proc_dir, &bsp_luna_wdt_regs_fops)){
      printk("create proc wdt/registers failed!\n");
      return;
    }
 #ifdef DEBUG_LOCK_TEST
      if(! proc_create("wdt_lock_test", 0444, watchdog_proc_dir, &bsp_luna_wdt_locktest_fops)){
      printk("create proc wdt/wdt_lock_test failed!\n");
      return;
    }
#endif
    
}

static void watchdog_dbg_exit(void)
{
#if 0
  /* Remove proc debug commands */
    if(watchdog_flag_entry)
    {
        remove_proc_entry("watchdog_flag", watchdog_flag_entry);
        watchdog_flag_entry = NULL;
    }
    if(wdg_timeout_entry)
    {
        remove_proc_entry("wdg_timeout", wdg_timeout_entry);
        wdg_timeout_entry = NULL;
    }
    if(kick_wdg_time_entry)
    {
        remove_proc_entry("kick_wdg_time", kick_wdg_time_entry);
        kick_wdg_time_entry = NULL;
    }
    if(wdg_register_entry)
    {
        remove_proc_entry("registers", wdg_register_entry);
        wdg_register_entry = NULL;
    }
#ifdef DEBUG_LOCK_TEST
    if(wdt_lock_entry)
    {
        remove_proc_entry("wdt_lock_test", wdt_lock_entry);
        wdt_lock_entry = NULL;
    }
#endif
    if(watchdog_proc_dir)
    {
        remove_proc_entry("watchdog", NULL);
        watchdog_proc_dir = NULL;
    }
#endif 
}

#if defined(CONFIG_DEFAULTS_KERNEL_3_18)
	#define __SRAM_PREDATA	__attribute__  ((section(".sram_predata")))
	#define __SRAM_TEXT	__attribute__  ((section(".sram_text")))
	#define __SRAM_DATA	__attribute__  ((section(".sram_data")))
#endif

#ifdef LUNA_RTL9602C
//static irqreturn_t watch_dog_timer_phase1_timeout_isr(int irq, void *dev_instance)
//irqreturn_t watch_dog_timer_phase1_timeout_isr(int irq, void *dev_instance)
__SRAM_TEXT static irqreturn_t watch_dog_timer_phase1_timeout_isr(int irq, void *dev_instance)
{
	REG32(0xb8003268) = 0x0;	
	printk("[BSP_WDT_PH1TO_IRQ #%d] ...\n", irq);

	REG32(0xb8000204) = 0x8000000A;

	//printk("\n\n[After DDR PLL config]\n");
	REG32(0xbb000104) = 0x4;

    	while (1) ;

	return IRQ_HANDLED;
}
#endif /* #ifdef LUNA_RTL9602C */


int __init watchdog_timeout_init(void)
{
    int i = 0;


#ifdef LUNA_RTL9602C
	int err = 0;
	err = request_irq(BSP_WDT_PH1TO_IRQ, watch_dog_timer_phase1_timeout_isr, 0, "Watch Dog Timer Phase1 Timeout", NULL );

	if(err)
		printk("[BSP_WDT_PH1TO_IRQ] Request IRQ ERROR !!!!!!");
#endif /* #ifdef LUNA_RTL9602C */

    if(CONFIG_WDT_ENABLE == 1){
        wdt_thread_maintain(1);
        set_rtk_wdt_ph1_threshold(wdg_timeout);
        RTK_WDG_EN();
        watchdog_flag = 1;
    }
    watchdog_dbg_init();
    show_usage();
#ifdef DEBUG
    printk("=================================\n");
    printk("%s\n", __FUNCTION__);
    for(i=0; i<3; i++) {
         printk("REG32(0x%08x)=0x%08x\n", (0xb8003260+(i*4)), REG32((0xb8003260+(i*4))));
    }
    printk("=================================\n");
#endif
    return 0;
}

void __exit watchdog_timeout_exit(void)
{
    printk("[%s-%d] exit!\n", __FILE__, __LINE__);
    RTK_WDG_DIS();
    wdt_thread_maintain(0);
    watchdog_dbg_exit();
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek watchdog timeout module");
MODULE_AUTHOR("David Chen <david_cw@realtek.com>");
module_init(watchdog_timeout_init);
module_exit(watchdog_timeout_exit);
