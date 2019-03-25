#include <linux/module.h>
#include <linux/proc_fs.h>

#include <asm/wmpu.h>

MODULE_LICENSE("Dual BSD/GPL");

static struct proc_dir_entry *proc_watchpoints = NULL;

#if 0
static struct timer_list watch_timer;

void watch_kernel(void) {
	
	/* schedule */
	if(!timer_pending(&watch_timer)){
		mod_timer(&watch_timer, jiffies + HZ);
	}
}
#endif

static void writeC0WatchLo(int which, u32 val) {
	switch (which) {
	case 0: write_c0_watchlo0(val); break;
	case 1: write_c0_watchlo1(val); break;
	case 2: write_c0_watchlo2(val); break;
	case 3: write_c0_watchlo3(val); break;
	case 4: write_c0_watchlo4(val); break;
	case 5: write_c0_watchlo5(val); break;
	case 6: write_c0_watchlo6(val); break;
	case 7: write_c0_watchlo7(val); break;
	}
}

static void writeC0WatchHi(int which, u32 val) {
	switch (which) {
	case 0: write_c0_watchhi0(val); break;
	case 1: write_c0_watchhi1(val); break;
	case 2: write_c0_watchhi2(val); break;
	case 3: write_c0_watchhi3(val); break;
	case 4: write_c0_watchhi4(val); break;
	case 5: write_c0_watchhi5(val); break;
	case 6: write_c0_watchhi6(val); break;
	case 7: write_c0_watchhi7(val); break;
	}
}

static void writeC0Wmpxmask(int which, u32 val) {
	switch (which) {
	case 0: write_lxc0_wmpxmask0(val); break;
	case 1: write_lxc0_wmpxmask1(val); break;
	case 2: write_lxc0_wmpxmask2(val); break;
	case 3: write_lxc0_wmpxmask3(val); break;
	case 4: write_lxc0_wmpxmask4(val); break;
	case 5: write_lxc0_wmpxmask5(val); break;
	case 6: write_lxc0_wmpxmask6(val); break;
	case 7: write_lxc0_wmpxmask7(val); break;
	}
}

#if 0
/* always assumes start is 2^N */
static int setup_watch_range(u32 start, u32 size, u32 index) {
	u32 s, mask;
	
	if ((index > 7) || (size < 8))
		return index;

	for (s = 0x80000000; s > size; s = s >> 1) {
	}
	mask = s - 1;
	
	//printk("%s: start: %08x, size: %06x(%06x), index=%d\n", __FUNCTION__, start, s, size, index);
	writeC0WatchLo(index, start | 0x1);
	writeC0WatchHi(index, (mask &  0xFF8) | (1 << 30));
	writeC0Wmpxmask(index, mask & 0xfffff000);
	return setup_watch_range(start + s, size - s, index + 1);
}

static void setup_watch(void) 
{
	int x;
	x = setup_watch_range(0x80000000, ((u32)&_etext - 0x80000000), 0);
	write_lxc0_wmpctl( ((((1<<x)-1) << 16) & 0x00ff0000) | 0x02);
}

static void watch_process(void) 
{
	setup_watch();
}
#endif

static int watchpoints_proc_read(char *page, char **start,
                 off_t off, int count,
                 int *eof, void *data) 
{
	printk("0: %08x  %08x  %08x\n", (u32)read_c0_watchlo0(), (u32)read_c0_watchhi0(), (u32)read_lxc0_wmpxmask0());
	printk("1: %08x  %08x  %08x\n", (u32)read_c0_watchlo1(), (u32)read_c0_watchhi1(), (u32)read_lxc0_wmpxmask1());
	printk("2: %08x  %08x  %08x\n", (u32)read_c0_watchlo2(), (u32)read_c0_watchhi2(), (u32)read_lxc0_wmpxmask2());
	printk("3: %08x  %08x  %08x\n", (u32)read_c0_watchlo3(), (u32)read_c0_watchhi3(), (u32)read_lxc0_wmpxmask3());
	printk("4: %08x  %08x  %08x\n", (u32)read_c0_watchlo4(), (u32)read_c0_watchhi4(), (u32)read_lxc0_wmpxmask4());
	printk("5: %08x  %08x  %08x\n", (u32)read_c0_watchlo5(), (u32)read_c0_watchhi5(), (u32)read_lxc0_wmpxmask5());
	printk("6: %08x  %08x  %08x\n", (u32)read_c0_watchlo6(), (u32)read_c0_watchhi6(), (u32)read_lxc0_wmpxmask6());
	printk("7: %08x  %08x  %08x\n", (u32)read_c0_watchlo7(), (u32)read_c0_watchhi7(), (u32)read_lxc0_wmpxmask7());
	printk("lxc0 wmpctl   : %08x\n", (u32)read_lxc0_wmpctl());
	printk("lxc0 wmpstatus: %08x\n", (u32)read_lxc0_wmpstatus());
	printk("lxc0 wmpvaddr : %08x\n", (u32)read_lxc0_wmpvaddr());
	return 0;
}

#define CP0_WATCHHI_VALUE	0xc0000ff8
static int watchpoints_proc_write(struct file *file, const char *buffer,
                                    unsigned long count, void *data)
{
	unsigned long addr, entry;
	//char maddr[12];
	int val;
	char cmd[32]="", opt='h', arg1[12], arg2[12];

	if (buffer && !copy_from_user(cmd, buffer, sizeof(cmd) -1 )) {  
		//sscanf(cmd,"%c %s\n",&opt, maddr);
		sscanf(cmd, "%c %s %s\n", &opt, arg1, arg2);
		switch(opt) {
		case 'w':
			printk("%s(%d)\n", __FUNCTION__, __LINE__);
			entry = simple_strtoul(arg1, NULL, 16);
			addr = simple_strtoul(arg2, NULL, 16);
			printk("#%d: Watch(0x%08X)\n", entry, addr);
			switch(entry) {
			case 0:
				write_c0_watchlo0(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi0(CP0_WATCHHI_VALUE);
				break;
			case 1:
				write_c0_watchlo1(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi1(CP0_WATCHHI_VALUE);
				break;
			case 2:
				write_c0_watchlo2(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi2(CP0_WATCHHI_VALUE);
				break;
			case 3:
				write_c0_watchlo3(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi3(CP0_WATCHHI_VALUE);
				break;
			case 4:
				write_c0_watchlo4(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi4(CP0_WATCHHI_VALUE);
				break;
			case 5:
				write_c0_watchlo5(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi5(CP0_WATCHHI_VALUE);
				break;
			case 6:
				write_c0_watchlo6(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi6(CP0_WATCHHI_VALUE);
				break;
			case 7:
				write_c0_watchlo7(addr | ATTR_DW | ATTR_DR | ATTR_IX);
				write_c0_watchhi7(CP0_WATCHHI_VALUE);
				break;
			defult:
				printk("UNKNOWN Entry #%d\n", entry);
				break;
			}
			write_lxc0_wmpctl( (((1 << entry) << 16) & 0x00ff0000) | 0x02);
			//writeC0WatchHi(x, (0xFF8) | (1 << 30)| (1 << 31));
			//addr = 0x007ff000;
			//write_lxc0_wmpxmask0(addr);
			//write_lxc0_wmpctl( ((((1<<1)-1) << 16) & 0x00ff0000) | 0x02);
			break;
		default:
			printk("UNKNOWN command\n");
			break;
		}
		return count;
	}

	return -EFAULT;
}


#if defined(CONFIG_DEFAULTS_KERNEL_3_18)
static const struct file_operations watchpoints_proc_fops = {
	.owner      = THIS_MODULE,
	//.open       = watchpoints_proc_open,
	.read       = watchpoints_proc_read,
	.write      = watchpoints_proc_write,
	//.release    = watchpoints_proc_release,
};
#endif

static void create_watchpoints_proc(void){
#if defined(CONFIG_DEFAULTS_KERNEL_2_6)
	proc_watchpoints = create_proc_entry("watchpoints", 0666, NULL);
#else
	proc_watchpoints = proc_create("watchpoints", 0666, NULL, &watchpoints_proc_fops);
#endif
	if (!proc_watchpoints) {
		printk(KERN_INFO "Error creating watchpoints proc entry\n");
		return ;
	}

#if defined(CONFIG_DEFAULTS_KERNEL_2_6)
	proc_watchpoints->read_proc = watchpoints_proc_read;
	proc_watchpoints->write_proc = watchpoints_proc_write;
#endif
	printk(KERN_INFO "watchpoints proc entry initialized!\n");

	return;
}


static int __init watchpoints_init(void) 
{
	printk(KERN_ALERT "Init Watchpoints\n");
	create_watchpoints_proc();
#if 0
	struct proc_dir_entry *pe;

	int x;
	
	setup_watch();
	printk("Setup memory watchpoint\n");
	
	init_timer(&watch_timer);
	watch_timer.function = watch_process;
	pe = create_proc_read_entry("watchpoints",
                          0444, NULL,
                          proc_read_watchpoints,
                          NULL);
#endif	
	return 0;
}

static void __exit watchpoints_exit(void) 
{
	printk(KERN_ALERT "Exit Watchpoints\n");
}

module_init(watchpoints_init);
module_exit(watchpoints_exit);

