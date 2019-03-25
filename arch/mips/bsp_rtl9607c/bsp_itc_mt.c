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

/******************ITC Test**********************************/
#include <asm/mipsmtregs.h>
#include <asm/r4kcache.h>
#include <asm/cacheflush.h>
#include <asm/cacheops.h>

/****************L2 cache ********************************/
#define M_PERFCTL_EXL			(1	<<  0)
#define M_PERFCTL_KERNEL		(1	<<  1)
#define M_PERFCTL_SUPERVISOR		(1	<<  2)
#define M_PERFCTL_USER			(1	<<  3)
#define M_PERFCTL_INTERRUPT_ENABLE	(1	<<  4)
#define M_PERFCTL_EVENT(event)		(((event) & 0x3ff)  << 5)
#define M_PERFCTL_VPEID(vpe)		((vpe)	  << 16)

#define M_PERFCTL_MT_EN(filter)		((filter) << 20)

#define	   M_TC_EN_ALL			M_PERFCTL_MT_EN(0)
#define	   M_TC_EN_VPE			M_PERFCTL_MT_EN(1)
#define	   M_TC_EN_TC			M_PERFCTL_MT_EN(2)
#define M_PERFCTL_TCID(tcid)		((tcid)	  << 22)
#define M_PERFCTL_WIDE			(1	<< 30)
#define M_PERFCTL_MORE			(1	<< 31)
#define M_PERFCTL_TC			(1	<< 30)

#define M_PERFCTL_COUNT_EVENT_WHENEVER	(M_PERFCTL_EXL |		\
					M_PERFCTL_KERNEL |		\
					M_PERFCTL_USER |		\
					M_PERFCTL_SUPERVISOR |		\
					M_PERFCTL_INTERRUPT_ENABLE)

#define M_PERFCTL_COUNT_EVENT_WHENEVER_NIE	(M_PERFCTL_EXL |		\
					M_PERFCTL_KERNEL |		\
					M_PERFCTL_USER |		\
					M_PERFCTL_SUPERVISOR )

#define ITC_Bypass_View	        (0x0<<3)
#define ITC_Control_View	(0x1<<3)

#define ITC_EF_Sync_View	(0x2<<3)
#define ITC_EF_Try_View	        (0x3<<3)

#define ITC_PV_Sync_View	(0x4<<3)
#define ITC_PV_Try_View	        (0x5<<3)

#define BSP_ITC_LOCK  bsp_itc_lock(21, &flag)
#define BSP_ITC_UNLOCK  bsp_itc_unlock(16, &flag)

#define ITC_PhyAddr  0x9000000	/* 1K alignment */
int ITC_CELL = 0;
int EntryGrain = 0x3;
int ITC_PITCH = 0x400;		//1024 = 128*(1 << EntryGrain)

// int ITC_addr_map = 0x00000c00; //4KB Range
int ITC_AddrMask = 0x3;		//4KB

unsigned int *ITC_BlcokNC;
/*********************************************/
/* EntryGrain[2 : 0] = 1  =>> 256B          */
/* EntryGrain[2 : 0] = 0x5  =>> 4KB          */
/* EntryGrain[2 : 0] = 0x3  =>> 1KB          */
/*********************************************/
/*Use physical address to itc_bse */
static void bsp_itc_init(unsigned itc_base)
{
    if (itc_base & 0x3FF) {
	printk
	    ("ITC BaseAddress must be 1024B alignment, input(0x%4x)\n",
	     itc_base);
	return;
    }

    if (itc_base != 0) {
	/*
	 * Configure ITC mapping.  This code is very
	 * specific to the 34K core family, which uses
	 * a special mode bit ("ITC") in the ErrCtl
	 * register to enable access to ITC control
	 * registers via cache "tag" operations.
	 */
	unsigned long ectlval;
	unsigned long itcblkgrn;

	/* ErrCtl register is known as "ecc" to Linux */
	ectlval = read_c0_ecc();
	write_c0_ecc(ectlval | (0x1 << 26));
	ehb();
#define INDEX_0 (0x80000000)
#define INDEX_8 (0x80000008)
	/* Read "cache tag" for Dcache pseudo-index 8 */
	cache_op(Index_Load_Tag_D, INDEX_8);
	ehb();
	itcblkgrn = read_c0_dtaglo();
	printk("Got ITCAddressMap1 Register =0x%lx\n", itcblkgrn);
	itcblkgrn &= 0xfffe0000;

	/* 0x2000  32KB */
	itcblkgrn |= (ITC_AddrMask << 10);

	itcblkgrn |= EntryGrain;
	/* Stage in Tag register */
	write_c0_dtaglo(itcblkgrn);
	printk("Set ITCAddressMap1 Register =0x%lx\n", itcblkgrn);
	ehb();
	/* Write out to ITU with CACHE op */
	cache_op(Index_Store_Tag_D, INDEX_8);

	/* Now set base address, and turn ITC on with 0x1 bit */
	/* Physical_address + enable_bit                    */
	write_c0_dtaglo((itc_base & 0xfffffc00) | 0x1);
	ehb();
	/* Write out to ITU with CACHE op */
	cache_op(Index_Store_Tag_D, INDEX_0);
	write_c0_ecc(ectlval);
	ehb();
	printk("Set ITCAddressMap0 Register =0x%x\n",
	       (itc_base & 0xfffffc00) | 0x1);
	printk("Mapped %ld ITC cells starting at 0x%08x\n",
	       ((itcblkgrn & 0x7fe00000) >> 20), itc_base);

	ITC_CELL = ((itcblkgrn & 0x7fe00000) >> 20);

	ITC_BlcokNC = (unsigned int *) (CKSEG1ADDR(itc_base));
	printk("ITC_BlcokNC=%p, MapSize=%d\n", ITC_BlcokNC,
	       1024 * (ITC_AddrMask + 1));
    }
}

static void bsp_itc_entry_init(void)
{
    unsigned int *ITC_Cell;
    unsigned int offset;
    unsigned int *ITC_Cell_ByPass;
    int index = 0;
    unsigned int cell_tag;
    for (index = 0; index < 32; index++) {
	offset = index * ITC_PITCH;
	/* control_view to change tag */
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Control_View);
	cell_tag = *ITC_Cell;
	printk("[CPU%d  %s] Index: %d, %p, ITC_CELL_TAG=0x%x\n",
	       smp_processor_id(), __func__, index, ITC_Cell, cell_tag);

	/*******  FIFO *********************************************
         * FIFODepth[31:28] = 0x2, four-entry FIFO cells
         * FIFO[17] = 0x1 , FIFO type(0, single-entry semaphore cells
         * Default value = 0x20020000
         *********************************************************/
	if ((cell_tag & 0xFFFE0000) == 0x20020000) {
	    /* FIFO */
	    *ITC_Cell |= 0x1;
	    printk
		("[ bsp_itc_entry_init- index:%d, Addr:%p, type:FIFO      ] Inited ITC_CELL_TAG=0x%x\n",
		 index, ITC_Cell, *ITC_Cell);
	} else {
	    /* single-entry Semaphore cells. */
	    *ITC_Cell = 0;	//INIT semp
	    printk
		("[ bsp_itc_entry_init- index:%d, Addr:%p, type:Semaphore ] Inited ITC_CELL_TAG=0x%x\n",
		 index, ITC_Cell, *ITC_Cell);

	    /* ITC_Bypass_View to set default value */
	    /*****************************************
	     * default: 
	     *        1  , unlock 
	     *        0  , lock
	     *****************************************/
	    
	    ITC_Cell_ByPass = (unsigned int *) (((unsigned int) ITC_BlcokNC +  offset)
	                      | ITC_Bypass_View);
// 	    *ITC_Cell_ByPass = 1;
	    *ITC_Cell_ByPass = 0;// Init to lock status for test
	    printk("         Index: %d, ITC_Cell_ByPass(%p)=0x%x\n",
		   index, ITC_Cell_ByPass, *ITC_Cell_ByPass);
	}
    }
}

static void bsp_itc_entry_list(void)
{
    unsigned int *ITC_Cell;
    unsigned int offset;

    int i;
    for (i = 0; i < ITC_CELL; i++) {
	offset = i * ITC_PITCH;
	/* control_view to change tag */
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Control_View);
	printk("CELL(%d):%p   Tag = 0x%4x\n", i, ITC_Cell, *ITC_Cell);
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Bypass_View);
	printk("CELL(%d):%p  Data = 0x%4x\n", i, ITC_Cell, *ITC_Cell);
    }

}

static void bsp_itc_lock(int index, int *flag)
{
    unsigned int *ITC_Cell;
    unsigned int *ITC_Cell_ByPass;
    unsigned int offset;
    int tmp;
    offset = index * ITC_PITCH;
    printk("CPU%d: bsp_itc_lock: %d\n", smp_processor_id(), index);
    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_PV_Sync_View);
    ITC_Cell_ByPass =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Bypass_View);
    printk
	("[CPU%d : bsp_itc_lock] ITC_Cell | ITC_Bypass_View = %p, value=0x%x\n",
	 smp_processor_id(), ITC_Cell_ByPass, *ITC_Cell_ByPass);
    tmp = *ITC_Cell;
    printk
	("[CPU%d : bsp_itc_lock] ITC_Cell | ITC_PV_Sync_View = %p,tmp=%d\n",
	 smp_processor_id(), ITC_Cell, tmp);
    printk
	("[CPU%d : bsp_itc_lock] ITC_Cell | ITC_Bypass_View = %p, value=0x%x\n",
	 smp_processor_id(), ITC_Cell_ByPass, *ITC_Cell_ByPass);
    printk
	("[CPU%d : bsp_itc_lock] ITC_Cell | ITC_Bypass_View = %p, value=0x%x\n",
	 smp_processor_id(), ITC_Cell_ByPass, *ITC_Cell_ByPass);
    *flag = tmp;
}

static void bsp_itc_unlock(int index, int *flag)
{
    unsigned int *ITC_Cell;
    unsigned int offset;
    unsigned int *ITC_Cell_ByPass;
    int tmp;
    offset = index * ITC_PITCH;
    tmp = *flag;
    ITC_Cell_ByPass =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Bypass_View);
    printk
	("[CPU%d : bsp_itc_unlock] ITC_Cell | ITC_Bypass_View = %p, value=0x%x, tmp=%d\n",
	 smp_processor_id(), ITC_Cell_ByPass, *ITC_Cell_ByPass, tmp);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_PV_Sync_View);
    *ITC_Cell = tmp;
    printk
	("[CPU%d : bsp_itc_unlock] ITC_Cell | ITC_PV_Sync_View = %p, value=0x%x\n",
	 smp_processor_id(), ITC_Cell, *flag);
    printk
	("[CPU%d : bsp_itc_unlock] ITC_Cell | ITC_Bypass_View = %p, value=0x%x, tmp=%d\n",
	 smp_processor_id(), ITC_Cell_ByPass, *ITC_Cell_ByPass, tmp);

}

struct itc_count_s {
    int cpu;
    unsigned long long testitc_v;
    int date;
};

struct itc_count_s itc_count;
#define ITC_TX 4
#define ITC_TY 4
#define ITC_TESTINCR 0x10
unsigned long long itctest_data[ITC_TX][ITC_TY] = { {0} };	// 4 *4 * 8

static struct task_struct *testitc_threads[NR_CPUS];
struct itc_test_threaddata {
    unsigned int events;
    int cpu;
    unsigned long long cnt;
    int date;
};
static struct itc_test_threaddata itc_test_data[NR_CPUS];

void itc_filldata(unsigned long long value)
{
    int i, j;
    unsigned long long tmp;
    tmp = value + ITC_TESTINCR;
    for (i = 0; i < ITC_TX; i++) {
	for (j = 0; j < ITC_TY; j++) {
	    itctest_data[i][j] = tmp;
	    tmp++;
	}
    }
}

void itc_checkdata(unsigned long long value)
{
    int i, j;
    unsigned long long tmp;
    if (itctest_data[0][0] == 0)
	return;

    tmp = value + ITC_TESTINCR;
    for (i = 0; i < ITC_TX; i++) {
	for (j = 0; j < ITC_TY; j++) {
	    if (itctest_data[i][j] != tmp)
		printk
		    ("CPU: %d,ITC checkdata data[%d,%d]:%llu!=%llu )\n",
		     smp_processor_id(), i, j, itctest_data[i][j], tmp);
	    tmp++;
	}
    }

}

int testitc_seconds = 20;

static void itctest_perf(struct itc_test_threaddata *data)
{
    unsigned int flag;
    unsigned long long tmp;
    BSP_ITC_LOCK;
    itc_count.cpu = smp_processor_id();
    tmp = itc_count.testitc_v;

    itc_count.testitc_v++;
    itc_filldata(data->cnt);
    itc_checkdata(data->cnt);

    if ((tmp + 1) != itc_count.testitc_v) {
	printk("CPU: %d, %s :%llu!=%llu ,itc_count.cpu=%d\n",
	       smp_processor_id(), __func__, tmp + 1,
	       itc_count.testitc_v, itc_count.cpu);
    }

    BSP_ITC_UNLOCK;

    if ((data->cnt & 0xf) == 12) {
	printk("CPU%d: %s Running!\n", smp_processor_id(), __func__);
    }
}

static int test_itc(void *arg)
{

    struct itc_test_threaddata *data = arg;

    while (!kthread_should_stop()) {
	itctest_perf(data);
	data->cnt++;
	set_current_state(TASK_INTERRUPTIBLE);
	/* Now sleep between a min of 100-300us and a max of 1ms */
	schedule_timeout(testitc_seconds * HZ);
    }

    return 0;
}

static void bsp_itc_base_init_percpu(void *info)
{

    bsp_itc_init(ITC_PhyAddr);
    bsp_itc_entry_init();
}

static void bsp_itc_base_init(int entryg, int addrmask)
{
    int cpu = 0;
    if (entryg >= 0) {
      	EntryGrain = entryg;
	ITC_PITCH = 128 * (1 << EntryGrain);

    }

    if (addrmask >= 0) {
	ITC_AddrMask = addrmask;
    }

    printk
	("[CPU%d: %s] ITC_PITCH= %d (B), ITC_AddrMask=0x%x, MapRange=%d(KB)\n",
	 cpu, __func__, ITC_PITCH, ITC_AddrMask, (ITC_AddrMask + 1));
    for_each_online_cpu(cpu) {
#ifdef CONFIG_MIPS_MT_SMP
       if(cpu_data[cpu].vpe_id == 0 )
#endif
	smp_call_function_single(cpu, bsp_itc_base_init_percpu, NULL, 1);
    }
}

static void testitc_thread_init(int start)
{
    int cpu = 0;

    for_each_online_cpu(cpu) {
	if (cpu < start)
	    continue;

	testitc_threads[cpu] =
	    kthread_create(test_itc, &itc_test_data[cpu], "testITC/%d",
			   cpu);
	if (WARN_ON(!testitc_threads[cpu])) {
	    printk("CPU%d create testitc_threads failed!", cpu);
	    goto out_free;
	}

	kthread_bind(testitc_threads[cpu], cpu);
	wake_up_process(testitc_threads[cpu]);
    }

  out_free:
    return;

}

static void testitc_thread_init3(int start)
{
    int cpu = 0;

    for_each_online_cpu(cpu) {
	if (cpu == start)
	    continue;

	testitc_threads[cpu] =
	    kthread_create(test_itc, &itc_test_data[cpu], "testITC/%d",
			   cpu);
	if (WARN_ON(!testitc_threads[cpu])) {
	    printk("CPU%d create testitc_threads failed!", cpu);
	    goto out_free;
	}

	kthread_bind(testitc_threads[cpu], cpu);
	wake_up_process(testitc_threads[cpu]);
    }

  out_free:
    return;

}

static void testitc_thread_stop(void)
{
    int cpu = 0;
    for_each_online_cpu(cpu) {
	if (testitc_threads[cpu] != NULL) {
	    kthread_stop(testitc_threads[cpu]);
	}
    }
}

static void testitc_lock_forever(void)
{
    int flag;
    BSP_ITC_LOCK;
    testitc_thread_init(0);

}

static void testitc_lock_case3(void)
{
    int flag;
    BSP_ITC_LOCK;
    testitc_thread_init3(smp_processor_id());

}

static void testitc_lock_one_entry(int index)
{
    int flag;
    bsp_itc_lock(index, &flag);

}

static void testitc_unlock_one_entry(int index)
{
    int flag;
    flag = 1;
    printk("%s-%d: %d\n", __func__, __LINE__, index);
    bsp_itc_unlock(index, &flag);

}

/****FIFI TEST
 *  FIFO Index: 0
 *
 */
int testitc_fifo_sec = 30;

struct itc_perf_data {
    unsigned int events;
    int cpu;
    unsigned int count0;
    unsigned int count1;
};

struct itc_perf_data testitc_fifo_perfcount[NR_CPUS];

static struct task_struct *testitc_fifo_threads[NR_CPUS];

struct testitc_data_s{
    unsigned int index;
    unsigned int data;
    unsigned int status;
    unsigned int count; 
};

struct testitc_data_s testitc_data[NR_CPUS] =  { {0} };

#define Instructions_completed 	  1	//count0
#define ITC_stall_cycles         40	//count1
static void itc_test_fifo_perfcount_set(void)
{

    write_c0_perfctrl0(M_PERFCTL_EVENT(Instructions_completed) |
		       M_PERFCTL_COUNT_EVENT_WHENEVER_NIE);
    write_c0_perfctrl1(M_PERFCTL_EVENT(ITC_stall_cycles) |
		       M_PERFCTL_COUNT_EVENT_WHENEVER_NIE);
    instruction_hazard();
    write_c0_perfcntr0(0);	//reset
    write_c0_perfcntr1(0);	//reset
}

static void itc_perf_show(void)
{
    int cpu = smp_processor_id();
    testitc_fifo_perfcount[cpu].count0 = read_c0_perfcntr0();
    testitc_fifo_perfcount[cpu].count1 = read_c0_perfcntr1();
    printk
	("CPU%d: Instructions_completed(count0) = %u, ITC_stall_cycles(count1)  = %u\n",
	 smp_processor_id(), testitc_fifo_perfcount[cpu].count0,
	 testitc_fifo_perfcount[cpu].count1);
}

static void itc_perf_show1(void)
{
    int cpu = smp_processor_id();
    unsigned int tmp = 0;
    tmp = read_c0_perfcntr0();
    printk("CPU%d: Instructions_completed(count0) Add = %u, Current=%u\n",
	   smp_processor_id(), tmp - testitc_fifo_perfcount[cpu].count0,
	   tmp);
    testitc_fifo_perfcount[cpu].count0 = 0;

    tmp = read_c0_perfcntr1();
    printk("CPU%d: ITC_stall_cycles(count1)   ADD    = %u, Current=%u\n",
	   smp_processor_id(), tmp - testitc_fifo_perfcount[cpu].count1,
	   tmp);
    testitc_fifo_perfcount[cpu].count1 = 0;

}

static void itctest_fifo(int index)
{
    unsigned int *ITC_Cell;
    unsigned int offset;
//   unsigned int* ITC_Cell_ByPass;
    unsigned int cell_tag;
    unsigned int cell_data;
    unsigned long flag;
    offset = index * ITC_PITCH;
    local_irq_save(flag);
    /* control_view to change tag */
    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    cell_tag = *ITC_Cell;

    printk
	("[cpu%d]itctest_fifo: index: %d, %p, ITC_CELL_TAG=0x%x, NumOfData=%d\n",
	 smp_processor_id(), index, ITC_Cell, cell_tag,
	 (cell_tag >> 18) & 0x7);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_EF_Sync_View);
    itc_test_fifo_perfcount_set();
    itc_perf_show();
    local_irq_restore(flag);
    cell_data = *ITC_Cell;
    local_irq_save(flag);
    itc_perf_show1();
    printk("[cpu%d]itctest_fifo: index: %d, %p, ITC_CELL_DATA = 0x%x\n",
	   smp_processor_id(), index, ITC_Cell, cell_data);
    local_irq_restore(flag);

}

static int test_itc_fifo(void *arg)
{
    int index = 0;
//       bsp_itc_base_init(-1);
    itc_test_fifo_perfcount_set();
    printk("[CPU%d] %s\n", smp_processor_id(), __func__);
    while (!kthread_should_stop()) {
	itctest_fifo(index);
	index = (index + 1) & 0xf;
	set_current_state(TASK_INTERRUPTIBLE);
	/* Now sleep between a min of 100-300us and a max of 1ms */
	schedule_timeout(testitc_fifo_sec * HZ);
    }
    return 0;
}

static void testitc_FIFO_test(void)
{
    int cpu;
    cpu = 3;

    testitc_fifo_threads[cpu] =
	kthread_create(test_itc_fifo, NULL, "test_ITC_FIFO/%d", cpu);
    if (WARN_ON(!testitc_fifo_threads[cpu])) {
	printk("[CPU%d] Create CPU%d's test_itc_fifo failed!",
	       smp_processor_id(), cpu);
	goto out_free;
    }

    kthread_bind(testitc_fifo_threads[cpu], cpu);
    wake_up_process(testitc_fifo_threads[cpu]);
  out_free:
    return;

}

/**** semp
*
****************/
static void itctest_semp(int index)
{
    unsigned int *ITC_Cell;
    unsigned int offset;
//   unsigned int* ITC_Cell_ByPass;
    unsigned int cell_tag;
    unsigned int cell_data;
    unsigned long flag;
    offset = index * ITC_PITCH;
    local_irq_save(flag);
    /* control_view to change tag */
    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    cell_tag = *ITC_Cell;

    printk("[cpu%d]%s: index: %d, %p, ITC_CELL_TAG=0x%x\n",
	   smp_processor_id(), __func__, index, ITC_Cell, cell_tag);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_PV_Sync_View);
    itc_test_fifo_perfcount_set();
    itc_perf_show();
//     local_irq_restore(flag);
    cell_data = *ITC_Cell;
//     local_irq_save(flag);
    itc_perf_show1();
    printk("[cpu%d]%s: index: %d, %p, ITC_CELL_DATA = 0x%x\n",
	   smp_processor_id(), __func__, index, ITC_Cell, cell_data);
    local_irq_restore(flag);

}

static int test_itc_semp(void *arg)
{
    int index = 16;
    int cpu =  smp_processor_id();
    
    itc_test_fifo_perfcount_set();
    printk("[CPU%d] %s\n",cpu, __func__);
    while (!kthread_should_stop()) {
	itctest_semp(index);
	index = (index + 1) & 0x1f;
	set_current_state(TASK_INTERRUPTIBLE);
	/* Now sleep between a min of 100-300us and a max of 1ms */
	schedule_timeout(testitc_fifo_sec * HZ);
    }
    return 0;
}

static void testitc_semp_test(void)
{
    int cpu;
    cpu = 3;

    testitc_fifo_threads[cpu] =
	kthread_create(test_itc_semp, NULL, "test_ITC_semp/%d", cpu);
    if (WARN_ON(!testitc_fifo_threads[cpu])) {
	printk("[CPU%d] Create CPU%d's test_itc_semp failed!",
	       smp_processor_id(), cpu);
	goto out_free;
    }

    kthread_bind(testitc_fifo_threads[cpu], cpu);
    wake_up_process(testitc_fifo_threads[cpu]);
  out_free:
    return;

}

/****************************
 * ITC Proc for Write
 * ************************/
static ssize_t
bsp_testitc_write_proc(struct file *file, const char __user * buffer,
		       size_t count, loff_t * ppos)
{
    char tmpbuf[32];
    int para0 = 0;
    int para1 = -1;
    int para2 = -1;
    int para_count = 0;
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
    para_count = sscanf(tmpbuf, "%d %d 0x%x", &para0, &para1, &para2);

    printk("cpu(%d) input: para0 = %d, para1 = %d, para2 = 0x%x\n",
	   smp_processor_id(), para0, para1, para2);

    if (para_count == 1) {
	para1 = -1;
    }

    if (para0) {
	bsp_itc_base_init(para1, para2);

	if (para0 == 0x4) {
	    printk("cpu(%d), input %d: Just ITC Init!\n\n",
		   smp_processor_id(), para0);
	    goto out;
	}

	/***** FIFO TEST *************/
	if (para0 == 0x5) {
	    printk("cpu(%d), input %d: ITC FIFO TEST!\n\n",
		   smp_processor_id(), para0);
	    testitc_FIFO_test();
	    goto out;
	}
	/***** SEMP TEST *************/
	if (para0 == 0x6) {
	    printk("cpu(%d), input %d: ITC SEMP TEST!\n\n",
		   smp_processor_id(), para0);
	    testitc_semp_test();
	    goto out;
	}
	/****************************/

	if (para0 == 0x2) {
	    printk("cpu(%d), input %d: testitc_lock_forever!\n\n",
		   smp_processor_id(), para0);
	    testitc_lock_forever();
	    goto out;
	}
	if (para0 == 0x3) {
	    printk("cpu(%d), input %d: testitc_lock_case3!\n\n",
		   smp_processor_id(), para0);
	    testitc_lock_case3();
	    goto out;
	}

	if ((para0 > 0xf) && (para0 < 0x20)) {
	    printk("cpu(%d), input %d: one etnry(%d) lock!\n\n",
		   smp_processor_id(), para0, para0);
	    testitc_lock_one_entry(para0);
	    goto out;
	}

	if ((para0 > 0x10f) && (para0 < 0x120)) {
	    printk("cpu(%d), input %d: one etnry(%d) unlock!\n",
		   smp_processor_id(), para0, (para0 & 0xff));
	    testitc_unlock_one_entry((para0 & 0xff));
	    goto out;
	}

	if ((para0 > 0x1f)) {
	    printk("cpu(%d), input %d: Setting sleep seconds!\n\n",
		   smp_processor_id(), para0);
	    testitc_seconds = para0;
	}

	testitc_thread_init(0);
    } else {
	testitc_thread_stop();
    }

  out:
    return count;
}

static int bsp_testitc_proc_show(struct seq_file *m, void *v)
{

    unsigned long ectlval;
    unsigned long itcblkgrn;
    unsigned long itcbase;

    /* ErrCtl register is known as "ecc" to Linux */
    ectlval = read_c0_ecc();
    write_c0_ecc(ectlval | (0x1 << 26));
    ehb();
#define INDEX_0 (0x80000000)
#define INDEX_8 (0x80000008)
    /* Read "cache tag" for Dcache pseudo-index 8 */
    cache_op(Index_Load_Tag_D, INDEX_8);
    ehb();
    itcblkgrn = read_c0_dtaglo();
    printk("Got ITCAddressMap1 Register =0x%08lx\n", itcblkgrn);

    /*Read to ITU with CACHE op */
    cache_op(Index_Load_Tag_D, INDEX_0);
    ehb();
    itcbase = read_c0_dtaglo();
    printk("Got ITCAddressMap0 Register(BaseAddress) =0x%08lx\n",
	   itcblkgrn);

    write_c0_ecc(ectlval);
    ehb();

    seq_printf(m, "ITC BaseAddress : 0x%08lx\n", itcbase);
    seq_printf(m, "ITC BaseAddress : %s\n",
	       (itcbase & 0x1) ? "enable" : "disable");

    seq_printf(m, "ITCAddressMap1: 0x%08lx\n", itcblkgrn);
    seq_printf(m, "ITC NumEntries: 0x%lx\n", (itcblkgrn >> 20) & 0x7ff);
    seq_printf(m, "ITC AddrMask  : 0x%lx\n", (itcblkgrn >> 10) & 0x7f);
    seq_printf(m, "ITC EntryGrain: 0x%lx\n", (itcblkgrn & 0x3));

    return 0;
}

static int bsp_testitc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, bsp_testitc_proc_show, NULL);
}

static const struct file_operations bsp_testitc_proc_fops = {
    .open = bsp_testitc_proc_open,
    .read = seq_read,
    .write = bsp_testitc_write_proc,
    .llseek = seq_lseek,
    .release = single_release,
};

/*************ITC FIFO TEST**********************
 *
 ***********************************************/

static void bsp_itcfifo_write(int index, unsigned int data)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);
    printk("              Cell(%d) : FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

    ITC_Cell = 	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_EF_Sync_View);

    *ITC_Cell = data;

    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk("              Cell(%d) : FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

}

static void bsp_itcfifo_get(int index)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);
    printk("              Cell_TAG(%d) : FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_EF_Sync_View);

    ITC_CELL_DATA = *ITC_Cell;
    printk("              Cell(%d)=%p,  FIFO DATA: 0x%x\n", index,
	   ITC_Cell, ITC_CELL_DATA);

    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk("              Cell_TAG(%d): FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

}

static void bsp_itcfifo_read_first(int index)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);
    printk("              Cell_TAG(%d) : FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Bypass_View);

    ITC_CELL_DATA = *ITC_Cell;
    printk("              Cell(%d)=%p,  FIFO DATA: 0x%x\n", index,
	   ITC_Cell, ITC_CELL_DATA);

    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk("              Cell_TAG(%d): FIFOPtr=%d, FULL=%d, EMPTY=%d\n",
	   index, (ITC_CELL_DATA >> 18) & 0x7, (ITC_CELL_DATA >> 1) & 0x1,
	   ITC_CELL_DATA & 0x1);

}

/****************************
 * ITC Proc for Write
 * ************************/
static ssize_t
bsp_itcfifo_write_proc(struct file *file, const char __user * buffer,
		       size_t count, loff_t * ppos)
{
    char tmpbuf[32];
    unsigned int index = 0;
    unsigned int data = 0;
    unsigned int type = 0;

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
    i = sscanf(tmpbuf, "%d %d 0x%x", &type, &index, &data);

    printk("[CPU%d]input type:%u index:%u, data: %x \n",
	   smp_processor_id(), type, index, data);
    if (i < 2) {
	return -EINVAL;
    }

    if (type == 0) {
	bsp_itcfifo_read_first(index);
    } else if (type == 1) {
	bsp_itcfifo_get(index);
    } else if ((type == 2) && i == 3) {
	bsp_itcfifo_write(index, data);
    }

    return count;
}

static int bsp_itcfifo_proc_show(struct seq_file *m, void *v)
{

    unsigned int *ITC_Cell;
    unsigned int offset;

    int i;
    for (i = 0; i < ITC_CELL; i++) {
	offset = i * ITC_PITCH;
	/* control_view to change tag */
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Control_View);
	seq_printf(m, "CELL(%d):%p   Tag = 0x%4x\n", i, ITC_Cell,
		   *ITC_Cell);
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Bypass_View);
	seq_printf(m, "CELL(%d):%p  Data = 0x%4x\n", i, ITC_Cell,
		   *ITC_Cell);
    }

    return 0;
}

static int bsp_itcfifo_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, bsp_itcfifo_proc_show, NULL);
}

static const struct file_operations bsp_itcfifo_proc_fops = {
    .open = bsp_itcfifo_proc_open,
    .read = seq_read,
    .write = bsp_itcfifo_write_proc,
    .llseek = seq_lseek,
    .release = single_release,
};

/**************************************************/

/*************ITC Semp TEST**********************
 *
 ***********************************************/

static void bsp_itcsemp_write(int index, unsigned int data)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_PV_Sync_View);

    *ITC_Cell = data;

    printk
	("[CPU%d : %s] ITC_Cell | ITC_PV_Sync_View = %p,index=%d, Data <= 0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell, index, data);

}

static void bsp_itcsemp_get(int index)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_PV_Sync_View);

    ITC_CELL_DATA = *ITC_Cell;
    printk("              Cell(%d)=%p,   DATA: 0x%x\n", index, ITC_Cell,
	   ITC_CELL_DATA);

    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA);

}

static void bsp_itcsemp_read_first(int index)
{
    int offset = 0;

    unsigned int *ITC_Cell = NULL;
    unsigned int *ITC_Cell_Tag = NULL;
    unsigned int ITC_CELL_DATA = 0;
    offset = index * ITC_PITCH;
    ITC_Cell_Tag =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Control_View);
    ITC_CELL_DATA = *ITC_Cell_Tag;
    printk
	("[CPU%d : %s] ITC_Cell | ITC_Control_View = %p,index=%d, Tag=0x%x, type=%s\n",
	 smp_processor_id(), __func__, ITC_Cell_Tag, index, ITC_CELL_DATA,
	 (ITC_CELL_DATA & 0x20000) ? "FIFO" : "SEMP");

    ITC_Cell =
	(unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			  ITC_Bypass_View);

    ITC_CELL_DATA = *ITC_Cell;
    printk("  ITC_Bypass_View  Cell(%d)=%p,  DATA: 0x%x\n", index,
	   ITC_Cell, ITC_CELL_DATA);

}

/****************************
 * ITC Proc for Write
 * ************************/
static ssize_t
bsp_itcsemp_write_proc(struct file *file, const char __user * buffer,
		       size_t count, loff_t * ppos)
{
    char tmpbuf[32];
    unsigned int index = 0;
    unsigned int data = 0;
    unsigned int type = 0;

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
    i = sscanf(tmpbuf, "%d %d 0x%x", &type, &index, &data);

    printk("[CPU%d]input type:%u index:%u, data: %x \n",
	   smp_processor_id(), type, index, data);
    if (i < 2) {
	return -EINVAL;
    }

    if (type == 0) {
	bsp_itcsemp_read_first(index);
    } else if (type == 1) {
	bsp_itcsemp_get(index);
    } else if ((type == 2) && i == 3) {
	bsp_itcsemp_write(index, data);
    }

    return count;
}

static int bsp_itcsemp_proc_show(struct seq_file *m, void *v)
{
    unsigned int *ITC_Cell;
    unsigned int offset;

    int i;
    for (i = 0; i < ITC_CELL; i++) {
	offset = i * ITC_PITCH;
	/* control_view to change tag */
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Control_View);
	seq_printf(m, "CELL(%d):%p   Tag = 0x%4x, type=%s\n", i,
		   ITC_Cell, *ITC_Cell,
		   (*ITC_Cell) & 0x20000 ? "FIFO" : "Semaphore");
	ITC_Cell =
	    (unsigned int *) (((unsigned int) ITC_BlcokNC + offset) |
			      ITC_Bypass_View);
	seq_printf(m, "CELL(%d):%p  Data = 0x%4x\n", i, ITC_Cell,
		   *ITC_Cell);
    }

    return 0;
}

static int bsp_itcsemp_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, bsp_itcsemp_proc_show, NULL);
}

static const struct file_operations bsp_itcsemp_proc_fops = {
    .open = bsp_itcsemp_proc_open,
    .read = seq_read,
    .write = bsp_itcsemp_write_proc,
    .llseek = seq_lseek,
    .release = single_release,
};

/**************************************************/
static int __init init_modules(void)
{

    proc_create("bsp_ITC_test", 0444, NULL, &bsp_testitc_proc_fops);

    proc_create("bsp_ITC_fifo", 0444, NULL, &bsp_itcfifo_proc_fops);

    proc_create("bsp_ITC_semp", 0444, NULL, &bsp_itcsemp_proc_fops);

    return 0;
}

static void __exit exit_modules(void)
{

}

module_init(init_modules);
module_exit(exit_modules);
