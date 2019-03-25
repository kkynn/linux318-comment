/* alan test */

#include <linux/smp.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
static DEFINE_SPINLOCK(smp_test_lock);

static void backtrace_test_irq_callback(unsigned long data);

static DECLARE_TASKLET(backtrace_tasklet0, &backtrace_test_irq_callback, 0);
static DECLARE_TASKLET(backtrace_tasklet1, &backtrace_test_irq_callback, 0);

static void backtrace_tasklet_schedule(void)
{
    int cpu = 0;
    cpu = smp_processor_id();
  if (cpu){
	tasklet_schedule(&backtrace_tasklet1);
  }else{
	tasklet_schedule(&backtrace_tasklet0);
  }
}

static void backtrace_test_irq_callback(unsigned long data)
{
	int cpu = 0;
	unsigned long flags;
 	spin_lock_irqsave(&smp_test_lock, flags);
#if 1	
	printk("[CPU%d]spin is lock= %d\n", smp_processor_id(), spin_is_locked(&smp_test_lock));
	printk("[CPU%d]c0_status=%x\n",smp_processor_id(), read_c0_status());
	printk("[CPU%d]irqs_disabled=%x\n",smp_processor_id(), irqs_disabled());
#endif
	cpu = smp_processor_id();
	if (printk_ratelimit()){
	    printk("[CPU%d]backtrace_tasklet%d - Running.....\n",
		   cpu, cpu);  
// 	    dump_stack();
	}
// 	backtrace_test_irq();
}

void do_cpu_action_IPI(void *info){
  printk("[CPU%d]%s:%d\n",
	 smp_processor_id(), __func__, __LINE__);
  dump_stack();
  backtrace_tasklet_schedule();
}

void ipi_test(int cpuid){
  	unsigned long flags;
 	spin_lock(&smp_test_lock);
#if 1
	printk("[CPU%d]spin is lock= %d\n", smp_processor_id(), spin_is_locked(&smp_test_lock));
	printk("[CPU%d]c0_status=%x\n",smp_processor_id(), read_c0_status());
	printk("[CPU%d]irqs_disabled=%x\n",smp_processor_id(), irqs_disabled());
	printk("[CPU%d]%s:%d target CPU%d\n",
	 smp_processor_id(),__func__, __LINE__, cpuid);
#endif  
  smp_call_function_single(cpuid,
	 do_cpu_action_IPI,  NULL, 0);
      spin_lock_irqsave(&smp_test_lock, flags);
	
}



static int smp_test_write_proc(struct file *file, const char __user *buf,
                       unsigned long count, void *data)
{
   char tmpbuf[10];
  int num = 0;
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

    if (copy_from_user(tmpbuf, buf, count)) {
        printk("copy from user failed\n");
        return -EFAULT;
    }
    sscanf(tmpbuf, "%d", &num);
    ipi_test(num);
    return count;
}
static atomic_t __cpuinitdata count_start_flag = ATOMIC_INIT(0);
static atomic_t __cpuinitdata count_count_start = ATOMIC_INIT(0);
static atomic_t __cpuinitdata count_count_stop = ATOMIC_INIT(0);
static atomic_t __cpuinitdata count_reference = ATOMIC_INIT(0);
static int smp_test_read_proc(char *page, char **start, off_t off,
                       int count, int *eof, void *data)
{
    int len = 0;
	unsigned long flags;
	unsigned int initcount;
	
    local_irq_save(flags);
    	atomic_set(&count_reference, read_c0_count());
	atomic_set(&count_start_flag, 1);
	smp_wmb();

	/* Count will be initialised to current timer for all CPU's */
	initcount = read_c0_count();
    local_irq_restore(flags);
    
    
    
    len += sprintf(page + len, "[cpu%d]smp atomic_read(&count_reference)=%u ; initcount=%u\n", smp_processor_id(), atomic_read(&count_reference), initcount);
    *eof = 1;
    return len;
}



void smp_test_proc(void){
    struct proc_dir_entry *ent;

    ent = create_proc_entry("smp_test", S_IFREG | S_IRWXU, NULL);
    if (!ent) {
        printk("create proc failed\n");

    } else {
        ent->write = smp_test_write_proc;
        ent->read = smp_test_read_proc;
    }
}

static int __init init_modules(void)
{

  smp_test_proc();
    return 0;
}

static void __exit exit_modules(void)
{
    remove_proc_entry("smp_test", NULL);
}

module_init(init_modules);
module_exit(exit_modules);
