#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/timex.h>
#include <linux/capability.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>


#include <bspchip.h>
#define CMCC_GBM_TIME_FACTOR 360
DEFINE_SPINLOCK(gbm_lock);
atomic_t jem_test_count = ATOMIC_INIT(0);


#ifdef CONFIG_CSRC_R4K
#include <linux/clocksource.h>
#include <linux/init.h>

#include <asm/time.h>

static int cmcc_init =0;
static int cmcc_rating = 0;
static cycle_t c0_hpt_read(struct clocksource *cs)
{
        return read_c0_count();
}

static struct clocksource clocksource_mips = {
        .name           = "MIPS CMCC",
        .read           = c0_hpt_read,
        .mask           = CLOCKSOURCE_MASK(32),
        .flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};
#define  CMCC_RATING_VALUE   2000
static int  cmcc_init_r4k_clocksource(void)
{
	uint32_t calc_value = 0;
	if (cmcc_init == 0){
        /* Calculate a somewhat reasonable rating value */
		cmcc_rating =  CMCC_RATING_VALUE;
        clocksource_mips.rating = cmcc_rating;
        calc_value = CMCC_GBM_TIME_FACTOR*(mips_hpt_frequency/100);
        clocksource_register_hz(&clocksource_mips, calc_value);
		cmcc_init = 1;

	}else{
		if(cmcc_rating != (CMCC_RATING_VALUE) ){
		  cmcc_rating = CMCC_RATING_VALUE;
		  clocksource_change_rating(&clocksource_mips, cmcc_rating);
		}
	}
        return 0;
}

#endif

#ifdef CONFIG_CPU_RLX5281
static void change_timer(void)
{
    unsigned long flags;
    spin_lock_irqsave(&gbm_lock, flags);
    if (REG32(BSP_TC0INT) & BSP_TCIP) {
        REG32(BSP_TC0INT) |= (BSP_TCIP);
    }

    /* disable timer */
    REG32(BSP_TC0CTL) = 0x0;    /* disable timer before setting CDBR */

    /* initialize timer registers */
    REG32(BSP_TC0CTL) |= (BSP_DIVISOR) << 0;
    //REG32(BSP_TC0DATA) = ( (BSP_MHZ)*((BSP_DIVISOR)/HZ)) << 0;
    REG32(BSP_TC0DATA) =
        ((BSP_MHZ * CMCC_GBM_TIME_FACTOR) * ((BSP_DIVISOR) / HZ)) << 0;

    REG32(BSP_TC0CTL) |= BSP_TCEN | BSP_TCMODE_TIMER;
    REG32(BSP_TC0INT) |= BSP_TCIE;
    spin_unlock_irqrestore(&gbm_lock, flags);
}

static void restore_timer(void)
{
    unsigned long flags;
    spin_lock_irqsave(&gbm_lock, flags);
    if (REG32(BSP_TC0INT) & BSP_TCIP) {
        REG32(BSP_TC0INT) |= (BSP_TCIP);
    }

    /* disable timer */
    REG32(BSP_TC0CTL) = 0x0;    /* disable timer before setting CDBR */

    /* initialize timer registers */
    REG32(BSP_TC0CTL) |= (BSP_DIVISOR) << 0;
    REG32(BSP_TC0DATA) = ((BSP_MHZ) * ((BSP_DIVISOR) / HZ)) << 0;
    REG32(BSP_TC0CTL) |= BSP_TCEN | BSP_TCMODE_TIMER;
    REG32(BSP_TC0INT) |= BSP_TCIE;
    spin_unlock_irqrestore(&gbm_lock, flags);
}
#else
 #ifdef CONFIG_CPU_MIPSIA
static void change_timer(void){
	int i, offset;
	int TCDATA = 0;
	int TCCTL = 0;
	unsigned long flags;

	spin_lock_irqsave(&gbm_lock, flags);
	TCDATA = ( (CMCC_GBM_TIME_FACTOR * BSP_MHZ * 1000000)/ ((int) DIVISOR_APOLLOPRO * HZ) );
  	for(i=0; i<NR_CPUS && i<TC_MAX; i++){
	    offset = i*0x10;
	    if (REG32(APOLLOPRO_TC0INT + offset) & APOLLOPRO_TCIP){
	        REG32(APOLLOPRO_TC0INT + offset) |= APOLLOPRO_TCIP;
	    }
         /* disable timer before setting CDBR */
       	 REG32(APOLLOPRO_TC0CTL + offset) = 0; /* disable timer before setting CDBR */
         REG32(APOLLOPRO_TC0DATA + offset ) =  TCDATA;       	    
	}
        /* Enable timer for all CPU at one time. Let the count of all timer is near */
	  TCCTL = APOLLOPRO_TCEN | APOLLOPRO_TCMODE_TIMER | DIVISOR_APOLLOPRO ;
     for(i=0; i<NR_CPUS && i<TC_MAX; i++){
	   offset = i*0x10;
	   REG32(APOLLOPRO_TC0CTL+ offset) = TCCTL;
    }
	spin_unlock_irqrestore(&gbm_lock, flags);		
#if defined(CONFIG_CSRC_R4K)
	cmcc_init_r4k_clocksource();
#endif
}
#ifdef CONFIG_CSRC_R4K	
#include <linux/workqueue.h>
#include <linux/slab.h>
static struct work_struct *cmcc_clocksource_work; // for event

static void cmcc_clocksource_routine(struct work_struct *work){
	clocksource_change_rating(&clocksource_mips, cmcc_rating);
}


static void  cmcc_change_rating(void){
	cmcc_rating = 100;
   cmcc_clocksource_work = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);
    INIT_WORK(cmcc_clocksource_work, cmcc_clocksource_routine);
    schedule_work(cmcc_clocksource_work);
}	
#endif	
static void restore_timer(void){
	int i, offset;
    int TCDATA = 0;
	int TCCTL = 0;
	unsigned long flags;
    spin_lock_irqsave(&gbm_lock, flags);
	TCDATA = ( (BSP_MHZ * 1000000)/ ((int) DIVISOR_APOLLOPRO * HZ) );
  	for(i=0; i<NR_CPUS && i<TC_MAX; i++){
	    offset = i*0x10;
	    if (REG32(APOLLOPRO_TC0INT + offset) & APOLLOPRO_TCIP){
	        REG32(APOLLOPRO_TC0INT + offset) |= APOLLOPRO_TCIP;
	    }
         /* disable timer before setting CDBR */
       	 REG32(APOLLOPRO_TC0CTL + offset) = 0; /* disable timer before setting CDBR */
         REG32(APOLLOPRO_TC0DATA + offset ) =  TCDATA;       	    
	}
        /* Enable timer for all CPU at one time. Let the count of all timer is near */
	  TCCTL = APOLLOPRO_TCEN | APOLLOPRO_TCMODE_TIMER | DIVISOR_APOLLOPRO ;
     for(i=0; i<NR_CPUS && i<TC_MAX; i++){
	   offset = i*0x10;
	   REG32(APOLLOPRO_TC0CTL+ offset) = TCCTL;
    }
	spin_unlock_irqrestore(&gbm_lock, flags);
#ifdef CONFIG_CSRC_R4K	
	cmcc_change_rating();
#endif
}
		
	
  #else	
  #error "Unknown Platform."	
  #endif
#endif


static pid_t gbm = 0;
static struct timer_list jem_timeout_timer;
static int old_value;
static DEFINE_MUTEX(cmcc_jem_mutex);
static void jem_vpe_maintenance(unsigned long args)
{
    if (old_value != atomic_read(&jem_test_count)) {
        old_value = atomic_read(&jem_test_count);
        mod_timer(&jem_timeout_timer, jiffies + (10 * HZ));
    } else {
        gbm = 0;
        restore_timer();
        atomic_set(&jem_test_count, 0);
    }
}

static void jem_start_timer(void)
{
    // 定義 timer Delay 時間
    jem_timeout_timer.expires = jiffies + (10 * HZ);    //30 second
    // 啟動 Timer
    add_timer(&jem_timeout_timer);

    return;
}


static void cmcc_jem_bad(struct timeval *tv, struct timeval *ktv)
{
    struct timeval user_tv;

    if ((strncmp(current->comm, "java", 4) == 0)
        && (copy_from_user(&user_tv, tv, sizeof(*tv)) == 0)
        && (user_tv.tv_sec == (unsigned int) current->pid)) {
        mutex_lock(&cmcc_jem_mutex);
        if (gbm != user_tv.tv_sec) {
            gbm = user_tv.tv_sec;
            change_timer();
            if (!timer_pending(&jem_timeout_timer)) {
                jem_start_timer();
            }

        }
        mutex_unlock(&cmcc_jem_mutex);
        atomic_inc(&jem_test_count);
    }
}

extern void (*cmcc_internal_function) (struct timeval * tv,
                                       struct timeval * ktv);

static int __init cmcc_jem_init(void)
{   
    // Timer init
    init_timer(&jem_timeout_timer);
    // 定義 timer 所執行之函式
    jem_timeout_timer.function = &jem_vpe_maintenance;
    // 定義 timer 傳入函式之 Data
    jem_timeout_timer.data = (0);
	
    cmcc_internal_function = &cmcc_jem_bad;
    return 0;
}

static void __exit cmcc_jem_exit(void)
{
    cmcc_internal_function = NULL;
}

module_init(cmcc_jem_init);
module_exit(cmcc_jem_exit);


MODULE_LICENSE("GPL");


