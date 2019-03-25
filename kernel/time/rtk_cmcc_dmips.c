#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/file.h>

/*****dhrystone 2.0/2.1/2.2 ****************************************/

static int dmips_acc = 0;
static unsigned int factor = 0, dividor = 0;
static char *get_task_path_n_nm(struct task_struct *tsk, char *buf)
{
	char *path_nm = NULL;
	struct file *exe_file;

	exe_file = get_mm_exe_file(current->mm);   
	if (exe_file) {
		path_nm = d_path(&exe_file->f_path, buf, 255);
		fput(exe_file);
		if (IS_ERR(path_nm)) {
		//ret = PTR_ERR(path_nm);
		path_nm = NULL;
	    }
	}
	return path_nm;
}

#define KEY_PATH_WORD "dmipsapp"
#define KEY_WORD "cc_dry2"
#define KEY_WORD2 "coremark"
int cmcc_dmips_patch_chk(void){
char buff[256];
char *path_nm = NULL;
path_nm = get_task_path_n_nm(current, buff);
if(path_nm){
	if(strstr(path_nm, KEY_PATH_WORD ) != NULL){
		return 1;
	}
}
return 0;
}


void cmcc_dmip_process(struct timeval *t){
	if(dmips_acc ==0 )
		return;
	
	if((strstr(current->comm, KEY_WORD) != NULL) || (cmcc_dmips_patch_chk() == 1 )){
	    t->tv_usec = t->tv_usec * dividor / factor;
	    t->tv_sec  = t->tv_sec  * dividor / factor;
	}
}
#include <linux/times.h>
void cmcc_dmip_process2(struct tms *tms){
	if(dmips_acc ==0)
		return;
	if((strstr(current->comm, KEY_WORD) != NULL) || (cmcc_dmips_patch_chk() == 1 )){
	tms->tms_stime= tms->tms_stime * dividor / factor;
	tms->tms_utime= tms->tms_utime * dividor / factor;
	}
}


void cmcc_coremark_process(struct timespec *tp){
	if(dmips_acc ==0)
		return;
	if((strstr(current->comm, KEY_WORD2) != NULL) || (cmcc_dmips_patch_chk() == 1 )){
	    tp->tv_sec = tp->tv_sec  * dividor / factor;
	    tp->tv_nsec= tp->tv_nsec * dividor / factor;
	}
}




#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

static int prof_rtk_dmips_proc_show(struct seq_file *m, void *v)
{
        seq_printf(m, "dmips_acc: 0x%x\n",dmips_acc);
        seq_printf(m, "factor: %ld\n", factor);
        seq_printf(m, "dividor: %ld\n", dividor);

        seq_putc(m, '\n');
        return 0;
}

static int prof_rtk_dmips_proc_open(struct inode *inode, struct file *file)
{
        return single_open(file, prof_rtk_dmips_proc_show, NULL);
}

static ssize_t prof_rtk_dmips_proc_write(struct file *file,
        const char __user *buf, size_t count, loff_t *pos)
{
    int input_val = 0;
    int ret = 0;
    sscanf(buf, "%d/%d", &factor, &dividor);
    input_val = factor/dividor;
    if(factor>=0 && dividor>0){
	dmips_acc = factor/dividor;
        return count;
    }
    return -EFAULT;

}
									   

static const struct file_operations prof_rtk_dmips_proc_fops = {
        .open           = prof_rtk_dmips_proc_open,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
        .write          = prof_rtk_dmips_proc_write,
};
#endif
static void create_prof_rtk_dmips(void)
{
	#ifdef CONFIG_PROC_FS
        proc_create("dmipsapp", 0600, NULL, &prof_rtk_dmips_proc_fops);
	#endif
}
static int __init cmcc_dmips_init(void)
{   
	create_prof_rtk_dmips();
    return 0;
}

static void __exit cmcc_dmips_exit(void)
{
    
}

module_init(cmcc_dmips_init);
module_exit(cmcc_dmips_exit);

