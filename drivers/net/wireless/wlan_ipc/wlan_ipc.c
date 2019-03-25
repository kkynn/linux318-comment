#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include "wlan_shm.h"

static	spinlock_t ipc_lock;

int aipc_wlan_set(int id, char* buf, int offset, int size, char sleep_en)
{
	int call_cnt = 0;
	
	if (offset < 0 || offset >= WLAN_BUFFER_SIZE){
		printk("aipc_wlan_set: offset is out of range(0~%d)\n", WLAN_BUFFER_SIZE);
		return -1;
	}

	if (size < 0 || (offset+size) > WLAN_BUFFER_SIZE){
		printk("aipc_wlan_set: size error\n");
		return -1;
	}

	spin_lock(&ipc_lock);

	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
	WLAN_SHM(id).lock_m = 1;
	WLAN_SHM(id).own = AIPC_WLAN_OWN_BY_SLAVE;
	#else
	WLAN_SHM(id).lock_s = 1;
	WLAN_SHM(id).own = AIPC_WLAN_OWN_BY_MASTER;
	#endif

	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
	while( (WLAN_SHM(id).lock_s==1 && WLAN_SHM(id).own==AIPC_WLAN_OWN_BY_SLAVE )){
	#else
	while( (WLAN_SHM(id).lock_m==1 && WLAN_SHM(id).own==AIPC_WLAN_OWN_BY_MASTER )){
	#endif
		if( sleep_en )
			schedule_timeout_interruptible(1);
		else
			mdelay(1);

		if( (call_cnt>500)){
			printk("wlan ipc timeout!\n");
			break;
		}
		call_cnt++;
	}

	if (call_cnt>500){
		spin_unlock(&ipc_lock);
		return -1;
	} else {
		memcpy( (void*)(&(WLAN_SHM(id).data.buf[offset])) , buf , size );		
	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
		WLAN_SHM(id).lock_m = 0;
	#else
		WLAN_SHM(id).lock_s = 0;
	#endif
		spin_unlock(&ipc_lock);
		return 0;
	}
}


int aipc_wlan_get(int id, char* buf, int offset, int size, char sleep_en)
{
	int call_cnt = 0;

	if (offset < 0 || offset >= WLAN_BUFFER_SIZE){
		printk("aipc_wlan_get: offset is out of range(0~2047)\n");
		return -1;
	}

	if (size < 0 || (offset+size) > WLAN_BUFFER_SIZE){
		printk("aipc_wlan_get: size error\n");
		return -1;
	}

	spin_lock(&ipc_lock);
	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
	WLAN_SHM(id).lock_m = 1;
	WLAN_SHM(id).own = AIPC_WLAN_OWN_BY_SLAVE;
	#else
	WLAN_SHM(id).lock_s = 1;
	WLAN_SHM(id).own = AIPC_WLAN_OWN_BY_MASTER;
	#endif

	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
	while( (WLAN_SHM(id).lock_s==1 && WLAN_SHM(id).own==AIPC_WLAN_OWN_BY_SLAVE )){
	#else
	while( (WLAN_SHM(id).lock_m==1 && WLAN_SHM(id).own==AIPC_WLAN_OWN_BY_MASTER )){
	#endif
		if( sleep_en )
			schedule_timeout_interruptible(1);
		else
			mdelay(1);

		if (call_cnt>1000){
			printk("wlan ipc timeout!\n");
			break;
		}
		call_cnt++;
	}

	if (call_cnt>500){
		spin_unlock(&ipc_lock);
		return -1;
	} else {

		memcpy( buf, (void*)(&(WLAN_SHM(id).data.buf[offset])), size );	
	#if !defined(CONFIG_ARCH_LUNA_SLAVE)
		WLAN_SHM(id).lock_m = 0;
	#else
		WLAN_SHM(id).lock_s = 0;
	#endif
		spin_unlock(&ipc_lock);
		return 0;
	}
}

static int proc_wlan_ipc_write(struct file *file, const char *buff,
                      unsigned long count, void *data)
{
	char 			tmpbuf[64], wbuf[8];
	unsigned int	offset, value, id;
	char			*strptr, *cmd_addr, *tokptr;

	if (buff && !copy_from_user(tmpbuf, buff, count)){
		tmpbuf[count] = '\0';
		strptr = tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
			goto errout;

		if (!memcmp(cmd_addr, "w", 1)){
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			id = (unsigned int)simple_strtol(tokptr, NULL, 0);
			if (id!=0 && id!=1)
				goto errout;

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			offset = simple_strtol(tokptr, NULL, 0);
			if (offset<0 || offset>=WLAN_BUFFER_SIZE)
				goto errout;

			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;

			value = simple_strtol(tokptr, NULL, 0);
			memcpy((void*)wbuf, (void*)&value, sizeof(unsigned int));
			aipc_wlan_set(id, wbuf, offset, sizeof(unsigned int), 1);
		} 
		else if (!memcmp(cmd_addr, "init", 4)){
			memset(&WLAN_SHM(0), 0, WLAN_IPC_MEM_SIZE);
			memset(&WLAN_SHM(1), 0, WLAN_IPC_MEM_SIZE);
		}
		else {
			goto errout;
		}
	}
	else {
errout:
		printk("id: (0|1), offset: 0~1999, value: 0~0xFFFFFFFF\n");
		printk("Write format(Only 4 bytes:  \"w id addr value\"\n");
		printk("Init  format:               \"init\"\n");
	}
	return count;
}

static int proc_wlan_ipc_read(struct seq_file *seq, void *v)
{
	int i;
	seq_printf(seq, "WLAN0 Shared Memory:\n");
	seq_printf(seq, "Master Lock Value: %d\n", WLAN_SHM(0).lock_m);
	seq_printf(seq, "Slave Lock Value:  %d\n", WLAN_SHM(0).lock_s);
	seq_printf(seq, "Own Bit:           %d\n", WLAN_SHM(0).own);
	seq_printf(seq, "Data Value:\n");
	for (i=0; i<WLAN_BUFFER_SIZE; i++){
		if ((i%16)==0){
			seq_printf(seq, "%08x: ", &WLAN_SHM(0).data.buf[i]);
		}
		seq_printf(seq, "%02x ", (unsigned char)WLAN_SHM(0).data.buf[i]);
		if ((i%16)==15){
			seq_printf(seq, "\n");
		}
	}
	
	seq_printf(seq, "-----------------------------------------------\n");

	seq_printf(seq, "WLAN1 Shared Memory:\n");
	seq_printf(seq, "Master Lock Value: %d\n", WLAN_SHM(1).lock_m);
	seq_printf(seq, "Slave Lock Value:  %d\n", WLAN_SHM(1).lock_s);
	seq_printf(seq, "Own Bit:           %d\n", WLAN_SHM(1).own);
	seq_printf(seq, "Data Value:\n");
	for (i=0; i<WLAN_BUFFER_SIZE; i++){
		if ((i%16)==0){
			seq_printf(seq, "%08x: ", &WLAN_SHM(1).data.buf[i]);
		}
		seq_printf(seq, "%02x ", (unsigned char)WLAN_SHM(1).data.buf[i]);
		if ((i%16)==15){
			seq_printf(seq, "\n");
		}
	}
	return 0;
}

static int proc_wlan_ipc_open(struct inode *inode, struct file *file)
{
        return single_open(file, proc_wlan_ipc_read, inode->i_private);
}

struct file_operations proc_wlan_ipc_fops=
{
	.open = proc_wlan_ipc_open,
	.read = seq_read,
	.write = proc_wlan_ipc_write,
	.owner = THIS_MODULE,
};

static int __init wlan_ipc_init(void)
{
#if defined(CONFIG_ARCH_LUNA_SLAVE)
	memset(&WLAN_SHM(0), 0, WLAN_IPC_MEM_SIZE);
	memset(&WLAN_SHM(1), 0, WLAN_IPC_MEM_SIZE);
#endif

	proc_create_data("wlan_ipc", 0, NULL, &proc_wlan_ipc_fops, NULL);

	return 0;
}

static void __exit wlan_ipc_exit(void)
{
	printk("wlan_ipc_exit\n");
}


EXPORT_SYMBOL( aipc_wlan_set );
EXPORT_SYMBOL( aipc_wlan_get );

late_initcall(wlan_ipc_init);
module_exit(wlan_ipc_exit);
