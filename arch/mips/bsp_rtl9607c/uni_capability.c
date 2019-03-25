#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

extern struct proc_dir_entry *realtek_proc;
#define PORT_NUMBER 5
int uni_port[PORT_NUMBER] = {0};
int logical_port[PORT_NUMBER] = {-1};
int isConfigured = 0;

#define _DEBUG 0

static int parsePort(char *src, int *output)
{
	int count=0;
	char *strptr = NULL, *tokptr = NULL;
#if _DEBUG
	printk("src %s\n", src);
#endif

	strptr = src;
	tokptr = strsep(&strptr, ";");
	while(strptr!=NULL){
		if(count < PORT_NUMBER){
			output[count] = simple_strtol(tokptr, NULL, 0);
#if _DEBUG
			printk("output[%d]=%d\n", count, output[count]);
#endif
		}
		else{
			printk("parse port error!!\n");
			return -1;
		}
		count++;
		tokptr = strsep(&strptr, ";");
	}
	return count;
}

static int checkSequence(int *src, int num)
{
	int i=0, j, isFound;
	for(i=0; i < num; i++){
		isFound = 0;
		for(j=0; j < num; j++){
			if(src[j]==i){
				isFound = 1;
				break;
			}
		}
		if(isFound==0){
			return 0;
		}
	}
	return 1;
}

static int uni_capability_proc_read(struct seq_file *s, void *v)
{
	int i = 0, j=0;
	char tmpbuf[128] = "\0";
	seq_printf(s, "# port capability\n");
	seq_printf(s, "# From left, physical uni 0, uni 1, uni 2...\n");
	seq_printf(s, "# GE 2, FE 1, None 0\n");
	seq_printf(s, "%s\n", CONFIG_UNI_CAPABILITY);
	seq_printf(s, "# logical port remapping\n");
	seq_printf(s, "# -1: invalid port\n");
	if(isConfigured==0){
		//parse uni port string
		memset(uni_port, 0, sizeof(int)*PORT_NUMBER);
		strcpy(tmpbuf, CONFIG_UNI_CAPABILITY);
		tmpbuf[strlen(tmpbuf)]=';';
		tmpbuf[strlen(tmpbuf)+1]='\0';
		parsePort(tmpbuf, uni_port);

		//fill logical port
		memset(logical_port, -1, sizeof(int)*PORT_NUMBER);
		for(i=0; i < PORT_NUMBER; i++){
			if(uni_port[i] != 0){
				logical_port[i] = j;
				j++;
			}
		}
		isConfigured = 1;
	}
	else{
		for(i=0; i < PORT_NUMBER; i++){
			if(i<PORT_NUMBER-1)
				seq_printf(s, "%d;", logical_port[i]);
			else
				seq_printf(s, "%d\n", logical_port[i]);
		}
	}
	return 0;
}


static int uni_capability_single_open(struct inode *inode, struct file *file)
{
	return(single_open(file, uni_capability_proc_read, NULL));
}

static int uni_capability_single_write(struct file *filp, const char *buffer, size_t count, loff_t *offp)
{
	char tmpbuf[128] = "\0";
	char tmpbuf1[128] = "\0";
	char key[20] = "\0";
	char *strptr = NULL, *tokptr = NULL;
	int i = 0, j=0;
	int port[PORT_NUMBER] = {-1};
	int logical_num = 0;
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		strptr = tmpbuf;
		tokptr = strsep(&strptr, " ");
#if _DEBUG
		printk("tokptr is %s\n", tokptr);
#endif
		if (!tokptr)
			strcpy(key, "");
		else
			strcpy(key, tokptr);
		if( strcmp(key, "REMAP")!=0){
			printk("parameter error, The logical port number is not matched!!\n");
			printk("echo [CMD REMAP]  [Logical port remppaing UNI0,UNI1,¡K,UNI3] > /proc/Realtek/uni_capability\n");
			printk("e.g. echo REMAP 2;0;1;3 > /proc/Realtek/uni_capability\n");
			return count;
		}
		tokptr = strsep(&strptr, "\n");
#if _DEBUG
		printk("tokptr is %s\n", tokptr);
#endif
		//parse logical port string
		strcpy(tmpbuf1, tokptr);
		tmpbuf1[strlen(tmpbuf1)]=';';
		tmpbuf1[strlen(tmpbuf1)+1]='\0';
		logical_num = parsePort(tmpbuf1, port);
#if _DEBUG
		printk("logical_num is %d\n", logical_num);
#endif
		//check logical port sequence
		if(checkSequence(port, logical_num)==0){
			printk("parameter error, The logical port sequence is not int the order!!\n");
			return count;
		}

		//parse uni port string
		memset(uni_port, 0, sizeof(int)*PORT_NUMBER);
		strcpy(tmpbuf1, CONFIG_UNI_CAPABILITY);
		tmpbuf1[strlen(tmpbuf1)]=';';
		tmpbuf1[strlen(tmpbuf1)+1]='\0';
		parsePort(tmpbuf1, uni_port);

		//fill logical port
		memset(logical_port, -1, sizeof(int)*PORT_NUMBER);
		for(i=0; i < PORT_NUMBER; i++){
			if(uni_port[i] != 0){
				if(port[j] != -1){
					logical_port[i] = port[j];
					j++;
				}
			}
		}
		if(j!=logical_num){
			printk("parameter error, The logical port number is not matched!!\n");
			return count;
		}
		else{
			isConfigured = 1;
			printk("Success!\n");
		}
	}
	return count;
}

static const struct file_operations fops_uni_capability_stats = {
	.open           = uni_capability_single_open,
	.read           = seq_read,
	.write          = uni_capability_single_write,
	.llseek         = seq_lseek,
	.release        = single_release,
};

static int __init uni_capability_init(void) {
	struct proc_dir_entry *pe;
	
	if(strlen(CONFIG_UNI_CAPABILITY)!=0){
		pe = proc_create("uni_capability", S_IRUSR |S_IWUSR | S_IRGRP | S_IROTH, realtek_proc,
                                                             &fops_uni_capability_stats);
		if (!pe) {
			return -EINVAL;
		}
	}
	
	return 0;
}

static void __exit uni_capability_exit(void) {
	return;
}

late_initcall_sync(uni_capability_init);
module_exit(uni_capability_exit); 

MODULE_LICENSE("GPL"); 
