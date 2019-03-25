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
 * Purpose : For 9607, CPU clock change
 *
 * Feature : Allow program to change CPU clock
 *
 */

 
/*
  * Include Files
  */
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <asm/mipsregs.h>

#include <bspchip.h>
#include <bspchip_8686.h>

//Soc System REGs
#define SYSREG_SYSTEM_STATUS_REG_A        (0xB8000044)
#define SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_FD_S  (2)
#define SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_MASK  (1 << SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_FD_S)
#define SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_OCP0PLL SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_MASK
#define SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_LXPLL (0 << SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_FD_S)

#define SYSREG_PIN_STATUS_REG_A           (0xB8000100)
#define SYSREG_DRAM_CLK_EN_FD_S         (0)
#define SYSREG_DRAM_CLK_EN_MASK         (1 << SYSREG_DRAM_CLK_EN_FD_S)
#define SYSREG_PIN_STATUS_CLSEL__FD_S       (5)
#define SYSREG_PIN_STATUS_CLSEL_MASK        (1 << SYSREG_PIN_STATUS_CLSEL__FD_S)
#define SYSREG_LX_DEFAULT_MHZ           (200)
#define SYSREG_MCKG_FREQ_DIV_FD_S           (0)
#define SYSREG_MCKG_FREQ_DIV_MASK           (0x3 << SYSREG_MCKG_FREQ_DIV_FD_S)

#define SYSREG_SYSCLK_CONTROL_REG_A       (0xB8000200)
#define SYSREG_MCKG_PHS_SEL_REG_A         (0xB8000220)
#define SYSREG_MCKG_FREQ_DIV_REG_A        (0xB8000224)
#define SYSREG_LX_PLL_SEL_REG_A           (0xB8000228)
#define SYSREG_MCKG_PHS_SEL_PHS_FD_S        (0)
#define SYSREG_MCKG_PHS_SEL_PHS_MASK        (0x7 << SYSREG_MCKG_PHS_SEL_PHS_FD_S)
#define SYSREG_SYSCLK_CONTROL_SDPLL_FD_S    (0)
#define SYSREG_SYSCLK_CONTROL_SDPLL_MASK    (0x1f << SYSREG_SYSCLK_CONTROL_SDPLL_FD_S)
#define SYSREG_SYSCLK_CONTROL_OCP1PLL_FD_S  (8)
#define SYSREG_SYSCLK_CONTROL_OCP1PLL_MASK  (0x1f << SYSREG_SYSCLK_CONTROL_OCP1PLL_FD_S)
#define SYSREG_SYSCLK_CONTROL_OCP0PLL_FD_S  (16)
#define SYSREG_SYSCLK_CONTROL_OCP0PLL_MASK  (0x1f << SYSREG_SYSCLK_CONTROL_OCP0PLL_FD_S)
#define SYSREG_DDRCKODL_DDRCLM_TAP_FD_S     (8)
#define SYSREG_DDRCKODL_DDRCLM_TAP_MASK     (0x1f << SYSREG_DDRCKODL_DDRCLM_TAP_FD_S)
#define SYSREG_DDRCKODL_PH90_TAP_FD_S       (16)
#define SYSREG_DDRCKODL_PH90_TAP_MASK       (0x1f << SYSREG_DDRCKODL_PH90_TAP_FD_S)

#define DEFAULT_CPU_TARGET   675

static inline void SYNC(void)
{
		__asm__ __volatile__(		
		".set	push\n\t"		
		".set	noreorder\n\t"		
 		"sync\n\t"
 		"nop\n\t"			
		".set	pop"			
		: /* no output */		
		: /* no input */		
		: "memory");
}


static inline u32 board_query_cpu_mhz(void){
    u32 tmp;
    tmp = (REG32(SYSREG_SYSCLK_CONTROL_REG_A) & SYSREG_SYSCLK_CONTROL_OCP0PLL_MASK)
             >> SYSREG_SYSCLK_CONTROL_OCP0PLL_FD_S ;

    if(REG32(SYSREG_PIN_STATUS_REG_A) & SYSREG_PIN_STATUS_CLSEL_MASK){
                return ((tmp + 25) * 20 );
     }
    else{
                return ((tmp + 20) * 25 );
    }
}

static inline unsigned long pll_gen1_is_CKSEL_25MHz(void)
{
	if(REG32(SYSREG_PIN_STATUS_REG_A) & SYSREG_PIN_STATUS_CLSEL_MASK)
		return 0;
	else
		return 1;
}

/* Function implementation */
/* Function Name:
 *      otto_pll_gen1_set
 * Descripton:
 *      Setting CPU/DSP/LX/DRAM PLL clock.
 * Input:
 *      sys_pll_ctl_oc25   : sys_pll_ctl using 25MHz OC.
 *      sys_pll_ctl_oc40   : sys_pll_ctl using 40MHz OC.
 *  sys_mckg_ph_sel    : MCKG phase deley value.
 *  sys_mckg_freq_div  : MCKG frequency division value.
 *  sys_lx_pll_sel_oc25: LX bus PLL selection value using 25MHz OC.
 *  sys_lx_pll_sel_oc40: LX bus PLL selection value using 40MHz OC.
 *  flag           : PLL_CPU_SET/PLL_DSP_SET/PLL_MEM_SET/PLL_LX_SET
 * Output:
 *      NONE
 * Return:
 *      NONE
 */
// #define DEBUG_TEST(fmt, args...) prom_printf( "DEBUG_TEST: " fmt, ## args)
#define DEBUG_TEST(fmt, args...) 
static unsigned long original_cpu_clk = 0;
noinline void otto_pll_setting(u32 target,  u32 sys_statusA_v1, u32 sys_statusA_v2 ) __attribute__ ((aligned (0x4000)));

noinline void otto_pll_setting(u32 target,  u32 sys_statusA_v1, u32 sys_statusA_v2 ){
    /*Critical Section*/
  
    REG32(SYSREG_SYSTEM_STATUS_REG_A) = sys_statusA_v1;
    udelay(5*1000);/* 5ms */
    REG32(SYSREG_SYSCLK_CONTROL_REG_A) = target;

    udelay(5*1000);/* 5ms */
    REG32(SYSREG_SYSTEM_STATUS_REG_A) = sys_statusA_v2;
    udelay(5*1000);/* 5ms */
    return; 
}

static unsigned int cal_target_sys_pll_ctl(u32 cpu_clk){
    unsigned long is_25MHz;
    unsigned long cpu_field;
    unsigned int target_sys_pll_ctl_tmp;
    /*
     * Note: We assume it's running in the SRAM on OCP0 bus now.
     */
    is_25MHz = pll_gen1_is_CKSEL_25MHz();
    
    /*
     * 1. LX clock flag checking
     */

    if(is_25MHz){
      	cpu_field = (cpu_clk/25)-20;
	target_sys_pll_ctl_tmp = (cpu_field << 16) ;
    }else{
      	cpu_field = (cpu_clk/20)-25;
	target_sys_pll_ctl_tmp = (cpu_field << 16) ;
    }
    return target_sys_pll_ctl_tmp;
  
}


static void otto_pll_gen1_set_cpu( unsigned long cpu_clk ) {
    //volatile unsigned int wait_loop_cnt;
    unsigned int target_sys_pll_ctl;


    u32 current_ctl;
    u32 sys_status_no_ocp0;
    u32 sys_status_ori;
    
    
    if( (cpu_clk <500) ||  (cpu_clk >800)){
      DEBUG_TEST("\n[Error]cpu_clk=%lu is not accpted!!\n", cpu_clk);
      return;
    }
    

    
    current_ctl = cal_target_sys_pll_ctl(board_query_cpu_mhz());
        
    target_sys_pll_ctl = cal_target_sys_pll_ctl(cpu_clk);

 DEBUG_TEST("cpu_clk=%lu, target_sys_pll_ctl=0x%8x\n", cpu_clk, target_sys_pll_ctl);
 DEBUG_TEST("[%s][%d] REG32(SYSREG_SYSTEM_STATUS_REG_A)=0x%8x\n", __func__, __LINE__,  REG32(SYSREG_SYSTEM_STATUS_REG_A) );
 
    sys_status_ori = REG32(SYSREG_SYSTEM_STATUS_REG_A) ;
    sys_status_no_ocp0 =  REG32(SYSREG_SYSTEM_STATUS_REG_A) & (~SYSREG_SYSTEM_STATUS_CF_CKSE_OCP0_MASK);
   /* 1. Prefetch  otto_pll_setting into icache */
    otto_pll_setting( current_ctl, sys_status_ori, sys_status_ori);
    
    udelay(5*1000);
    barrier();

    /* 2.  otto_pll_setting change to desired CPU Clock */
    otto_pll_setting(target_sys_pll_ctl, sys_status_no_ocp0, sys_status_ori);
   

 DEBUG_TEST("[%s][%d] REG32(SYSREG_SYSTEM_STATUS_REG_A)=0x%8x\n", __func__, __LINE__,  REG32(SYSREG_SYSTEM_STATUS_REG_A) );
    return;
}

static void cpu_clk_change(unsigned long cpu_clk){
  unsigned long irqflags;
  
  if(cpu_clk == board_query_cpu_mhz()){
	return;  
  }
  
  local_irq_save(irqflags);

  otto_pll_gen1_set_cpu(cpu_clk);

  local_irq_restore(irqflags);
  return;
}


void cpu_clock_switch(int input_val){
   switch (input_val){
     case 0:
       cpu_clk_change(original_cpu_clk);
       break;
     case 1:
       cpu_clk_change(DEFAULT_CPU_TARGET);
       break;
     default:
      cpu_clk_change(input_val);
       break; 
  }
  return;
}


static int test_switch_show(struct seq_file *seq, void *v)
{
  u32 cpu_freq = 0;
  u32 cctl_value =0;
  cctl_value = read_c0_cctl0()	;
  cpu_freq = board_query_cpu_mhz();
  
  seq_printf(seq, " CPU Clock : %u\n",cpu_freq );
  seq_printf(seq, " CCTL : 0x%08x\n", cctl_value);
   
    return 0;
}

static ssize_t test_switch_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
        printk("write value 0x%08x\n", input_val);
	    cpu_clock_switch(input_val);		
        return size;
    }
    return -EFAULT;
}


struct proc_dir_entry *test_proc_dir = NULL;

static int test_switch_open(struct inode *inode, struct file *file)
{
    return single_open(file, test_switch_show, inode->i_private);
}
static const struct file_operations bsp_luna_test_fops = {
    .owner      = THIS_MODULE,
    .open       = test_switch_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = test_switch_write,
};


int __init luna_test_init(void)
{
    original_cpu_clk = board_query_cpu_mhz();
    printk("[%s]CPU : %lu\n", __func__,  original_cpu_clk);
    test_proc_dir = proc_mkdir("luna_test", NULL);
    if (test_proc_dir == NULL) {
        printk("create /proc/luna_test failed!\n");
        return 1;
    }
    
    if(! proc_create("test", 0444, test_proc_dir, &bsp_luna_test_fops)){
      printk("create proc luna_test/test failed!\n");
      return 1;
    }

    return 0;
}

void __exit luna_test_exit(void)
{
     remove_proc_entry("test", test_proc_dir);
     proc_remove(test_proc_dir);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek TEST : ");
module_init(luna_test_init);
module_exit(luna_test_exit);
