#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>

#include <bspchip.h>

MODULE_LICENSE("Dual BSD/GPL");

static struct proc_dir_entry *proc_bus_timeout_monitor = NULL;

//int rlx_bus_timeout_monitor_interrupt()
static irqreturn_t rlx_bus_timeout_monitor_interrupt(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[%d]", irq);
#if 0
	switch(irq) {
		case BSP_LBCTMOm2_IRQ:
		case BSP_LBCTMOm1_IRQ:
		case BSP_LBCTMOm0_IRQ:
		case BSP_LBCTMOs3_IRQ:
		case BSP_LBCTMOs2_IRQ:
		case BSP_LBCTMOs1_IRQ:
		case BSP_LBCTMOs0_IRQ:
		case BSP_OCPTO1_IRQ  :
		case BSP_OCPTO0_IRQ  :
	}
#endif
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_ocp0(int irq, void * dev_instance, struct pt_regs *regs)
{
	//struct pt_regs *regs = get_irq_regs();
	printk("================================================\n");
	printk("[ocp0:%d] O0BTMAR = 0x%08X\n", irq, REG32(BSP_O0BTMAR));
	//printk("epc = 0x%08X\n", __read_32bit_c0_register($14, 0));
	printk("epc = 0x%08X\n", __read_ulong_c0_register($14, 0));
	printk("\n");

	//if (regs)
	//	show_regs(regs);
	dump_stack();
	printk("\n");
	REG32(BSP_O0BTIR) |= (1 << 31);			/* Clear interrupt */

	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_ocp1(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[ocp1:%d] ", irq);
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbm0(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbm0:%d] LB0MTMAR = 0x%08X\n", irq, REG32(BSP_LB0MTMAR));
	REG32(BSP_LB0MTIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbm1(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbm1:%d] LB1MTMAR = 0x%08X\n", irq, REG32(BSP_LB1MTMAR));
	REG32(BSP_LB1MTIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbm2(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbm2:%d] LB2MTMAR = 0x%08X\n", irq, REG32(BSP_LB2MTMAR));
	REG32(BSP_LB2MTIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbs0(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbs0:%d] LB0STMAR = 0x%08X\n", irq, REG32(BSP_LB0STMAR));
	REG32(BSP_LB0STIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbs1(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbs1:%d] LB1STMAR = 0x%08X\n", irq, REG32(BSP_LB1STMAR));
	REG32(BSP_LB1STIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbs2(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbs2:%d] LB2STMAR = 0x%08X\n", irq, REG32(BSP_LB2STMAR));
	REG32(BSP_LB2STIR) |= (1 << 31);		/* Clear interrupt */
	return 1;
}

static irqreturn_t rlx_bus_timeout_monitor_isr_lxbs3(int irq, void * dev_instance, struct pt_regs *regs)
{
	printk("[lxbs3: %d] ", irq);
	return 1;
}

static struct irqaction rlx_lxbus_timeout_monitor_m0_irq = {
	.handler	= rlx_bus_timeout_monitor_interrupt,
	.flags		= IRQF_DISABLED | IRQF_PERCPU,
	.name		= "lxbus timeout monitor",
};

/*
int __init rlx_clockevent_init(int irq)
{
	struct clock_event_device *cd;

	cd = &rlx_clockevent;
	cd->rating = 100;
	cd->irq = irq;
	clockevent_set_clock(cd, 32768);
	cd->max_delta_ns = clockevent_delta2ns(0x7fffffff, cd);
	cd->min_delta_ns = clockevent_delta2ns(0x300, cd);
	cd->cpumask = cpumask_of(0);

	clockevents_register_device(&rlx_lxbus_timeout_monitor);

	return setup_irq(irq, &rlx_irqaction);
}
*/

/*
void __init rlx_lxbus_timeout_monitor(void)
{
	//rlx_clockevent_init_(BSP_LBCTMOm0_IRQ);
	setup_irq(BSP_LBCTMOm0_IRQ, &rlx_lxbus_timeout_monitor_m0_irq);
}
*/

void bus_timeout_monitor_info(void){
	printk("\n"); 

	printk("OCP Bus Timeout Monitor Registers .....\n");
	printk("=================================\n");
	printk("BSP_O0BT	: 0x%08X\n", REG32(BSP_O0BT));
	printk("BSP_O0BTIR   	: 0x%08X\n", REG32(BSP_O0BTIR));
	printk("BSP_O0BTMAR  	: 0x%08X\n", REG32(BSP_O0BTMAR));

	printk("\n"); 
	printk("LX  Bus Timeout Monitor Registers\n");
	printk("=================================\n");
	printk("BSP_LBPSTCR	: 0x%08X\n", REG32(BSP_LBPSTCR));
	printk("BSP_LBPSTIR	: 0x%08X\n", REG32(BSP_LBPSTIR));
	printk("BSP_LBPSTMAR	: 0x%08X\n", REG32(BSP_LBPSTMAR));
                             
	printk("BSP_LB0MTCR	: 0x%08X\n", REG32(BSP_LB0MTCR));
	printk("BSP_LB0MTIR	: 0x%08X\n", REG32(BSP_LB0MTIR));
	printk("BSP_LB0MTMAR	: 0x%08X\n", REG32(BSP_LB0MTMAR));
                             
	printk("BSP_LB0STCR	: 0x%08X\n", REG32(BSP_LB0STCR));
	printk("BSP_LB0STIR	: 0x%08X\n", REG32(BSP_LB0STIR));
	printk("BSP_LB0STMAR	: 0x%08X\n", REG32(BSP_LB0STMAR));
                             
	printk("BSP_LB1MTCR	: 0x%08X\n", REG32(BSP_LB1MTCR));
	printk("BSP_LB1MTIR	: 0x%08X\n", REG32(BSP_LB1MTIR));
	printk("BSP_LB1MTMAR	: 0x%08X\n", REG32(BSP_LB1MTMAR));
                             
	printk("BSP_LB1STCR	: 0x%08X\n", REG32(BSP_LB1STCR));
	printk("BSP_LB1STIR	: 0x%08X\n", REG32(BSP_LB1STIR));
	printk("BSP_LB1STMAR	: 0x%08X\n", REG32(BSP_LB1STMAR));
                             
	printk("BSP_LB2MTCR	: 0x%08X\n", REG32(BSP_LB2MTCR));
	printk("BSP_LB2MTIR	: 0x%08X\n", REG32(BSP_LB2MTIR));
	printk("BSP_LB2MTMAR	: 0x%08X\n", REG32(BSP_LB2MTMAR));
                             
	printk("BSP_LB2STCR	: 0x%08X\n", REG32(BSP_LB2STCR));
	printk("BSP_LB2STIR	: 0x%08X\n", REG32(BSP_LB2STIR));
	printk("BSP_LB2STMAR	: 0x%08X\n", REG32(BSP_LB2STMAR));
                             
	printk("BSP_LBPUWTCR	: 0x%08X\n", REG32(BSP_LBPUWTCR));
	printk("BSP_LBPUWTIR	: 0x%08X\n", REG32(BSP_LBPUWTIR));
	printk("BSP_LBPUWTMAR	: 0x%08X\n", REG32(BSP_LBPUWTMAR));
                             
	printk("BSP_LBPURTCR	: 0x%08X\n", REG32(BSP_LBPURTCR));
	printk("BSP_LBPURTIR	: 0x%08X\n", REG32(BSP_LBPURTIR));
	printk("BSP_LBPURTMAR	: 0x%08X\n", REG32(BSP_LBPURTMAR));
                             
	printk("BSP_LBPDWTCR	: 0x%08X\n", REG32(BSP_LBPDWTCR));
	printk("BSP_LBPDWTIR	: 0x%08X\n", REG32(BSP_LBPDWTIR));
	printk("BSP_LBPDWTMAR	: 0x%08X\n", REG32(BSP_LBPDWTMAR));
                             
	printk("BSP_LBPDRTCR	: 0x%08X\n", REG32(BSP_LBPDRTCR));
	printk("BSP_LBPDRTIR	: 0x%08X\n", REG32(BSP_LBPDRTIR));
	printk("BSP_LBPDRTMAR	: 0x%08X\n", REG32(BSP_LBPDRTMAR));
	printk("\n"); 

	printk("epc = 0x%08X\n", __read_ulong_c0_register($14, 0));
	printk("\n"); 
}

static int bus_timeout_monitor_proc_read(char *buf,char **start,off_t offset,int count,int *eof,void *data )
{
	int len=0;
	//printk("%s(%d)\n", __FUNCTION__, __LINE__);

	//preempt_disable();
	bus_timeout_monitor_info();
	//preempt_enable();

	return len;
}

static int bus_timeout_monitor_proc_write(struct file *file, const char *buffer,
                                    unsigned long count, void *data)
{
	unsigned long addr;
	int val;
	char cmd[32]="", opt='h', arg1[12], arg2[12];

	if (buffer && !copy_from_user(cmd, buffer, sizeof(cmd) - 1 )) {  
		//printk("%s(%d): %s\n", __FUNCTION__, __LINE__, cmd);
		sscanf(cmd, "%c %s %s\n", &opt, arg1, arg2);
		switch(opt) {
		case 'r':
			addr = simple_strtoul(arg1, NULL, 16);
			printk("READ(0x%08X) = 0x%08x\n", addr, REG32(addr));
			//__asm__ __volatile__(
			//	"nop; nop; nop; nop;\n\t"
			//	"nop; nop; nop; nop;\n\t");
			break;
		case 'w':
			addr = simple_strtoul(arg1, NULL, 16);
			val = simple_strtoul(arg2, NULL, 16);
			printk("Write(0x%08x) = 0x%08X\n", addr, val);
			REG32(addr) = val;
			break;
		case 'h':
		default:
			printk("bus_timeout_monitor: UNKNOWN proc command: [%c] !!!\n", opt);
			break;
		}
		return count;
	}

	return -EFAULT;
}

#if defined(CONFIG_DEFAULTS_KERNEL_3_18)
static const struct file_operations bus_timeout_monitor_proc_fops = {
	.owner      = THIS_MODULE,
	//.open       = bus_timeout_monitor_proc_open,
	.read       = bus_timeout_monitor_proc_read,
	.write      = bus_timeout_monitor_proc_write,
};
#endif

static void create_bus_timeout_monitor_proc(void){
#if defined(CONFIG_DEFAULTS_KERNEL_2_6)
	proc_bus_timeout_monitor = create_proc_entry("bus_timeout_monitor", 0666, NULL);
#else
	proc_bus_timeout_monitor = proc_create("bus_timeout_monitor", 0666, NULL, &bus_timeout_monitor_proc_fops);
#endif
	if (!proc_bus_timeout_monitor) {
		printk(KERN_INFO "Error creating proc entry\n");
		return ;
	}

#if defined(CONFIG_DEFAULTS_KERNEL_2_6)
	proc_bus_timeout_monitor->read_proc = bus_timeout_monitor_proc_read;
	proc_bus_timeout_monitor->write_proc = bus_timeout_monitor_proc_write;
#endif
	printk(KERN_INFO "bus_timeout_monitor proc entry initialized!\n");

	return;
}

#define BSP_OBTCR_DEFAULT	((1 << 31) | (0 << 30) | (0b0111 << 26))
#define BSP_LBTCR_DEFAULT	((1 << 31) | (0b111 << 28))

static int bus_timeout_monitor_init(void)
{
	printk(KERN_ALERT "Init Bus Timeout Monitor\n");
	create_bus_timeout_monitor_proc();

#if 0
	REG32(BSP_O0BT) |= (1 << 31);
	REG32(BSP_LBPSTCR) |= (1 << 31);
	REG32(BSP_LB0MTCR) |= (1 << 31);	
	REG32(BSP_LB0STCR) |= (1 << 31);	
	REG32(BSP_LB1MTCR) |= (1 << 31);	
	REG32(BSP_LB1STCR) |= (1 << 31);	
	REG32(BSP_LB2MTCR) |= (1 << 31);	
	REG32(BSP_LB2STCR) |= (1 << 31);	
	REG32(BSP_LBPUWTCR) |= (1 << 31);	
	REG32(BSP_LBPURTCR) |= (1 << 31);	
	REG32(BSP_LBPDWTCR) |= (1 << 31);	
	REG32(BSP_LBPDRTCR) |= (1 << 31);	
#else
	REG32(BSP_O0BT) = BSP_OBTCR_DEFAULT;
	REG32(BSP_O0BTIR) | (1 << 31);			/* Clear interrupt */
	REG32(BSP_LBPSTCR) = BSP_LBTCR_DEFAULT;
	REG32(BSP_LB0MTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LB0STCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LB1MTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LB1STCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LB2MTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LB2STCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LBPUWTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LBPURTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LBPDWTCR) = BSP_LBTCR_DEFAULT;	
	REG32(BSP_LBPDRTCR) = BSP_LBTCR_DEFAULT;	
#endif

#if 1
	//setup_irq(BSP_LBCTMOm0_IRQ, &rlx_lxbus_timeout_monitor_m0_irq);

	request_irq(BSP_OCPTO0_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_ocp0, IRQF_DISABLED, "ocp timeout monitor 0", NULL);
	request_irq(BSP_OCPTO1_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_ocp1, IRQF_DISABLED, "ocp timeout monitor 1", NULL);

	request_irq(BSP_LBCTMOm0_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbm0, IRQF_DISABLED, "lxbus timeout monitor m0", NULL);
	request_irq(BSP_LBCTMOm1_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbm1, IRQF_DISABLED, "lxbus timeout monitor m1", NULL);
	request_irq(BSP_LBCTMOm2_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbm2, IRQF_DISABLED, "lxbus timeout monitor m2", NULL);
	request_irq(BSP_LBCTMOs0_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbs0, IRQF_DISABLED, "lxbus timeout monitor m0", NULL);
	request_irq(BSP_LBCTMOs1_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbs1, IRQF_DISABLED, "lxbus timeout monitor m1", NULL);
	request_irq(BSP_LBCTMOs2_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbs2, IRQF_DISABLED, "lxbus timeout monitor m2", NULL);
	request_irq(BSP_LBCTMOs3_IRQ, (irq_handler_t)rlx_bus_timeout_monitor_isr_lxbs3, IRQF_DISABLED, "lxbus timeout monitor m3", NULL);
#endif
	return 0;
}

static void bus_timeout_monitor_exit(void)
{
	printk(KERN_ALERT "Exit Bus Timeout Monitor\n");
}

module_init(bus_timeout_monitor_init);
module_exit(bus_timeout_monitor_exit);

