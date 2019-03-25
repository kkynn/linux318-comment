#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/atomic.h>
#include <linux/cgroup.h>
#include "sched.h"

#define CMCC_LIMIT_PROC_DIR    "rtk_cmcc_cpu_mem_limit"
#define CMCC_LIMIT_PROC_FILE   "limit_enable"

#define CMCC_LIMIT_SH     "/etc/scripts/cmcc_cpulimit_cgroup_maintain.sh"
#define CMCC_LIMIT_STOP_SH     "/etc/scripts/cmcc_cpulimit_stop.sh"
#define CMCC_LIMIT_INIT_SH     "/etc/scripts/cmcc_cpulimit_init.sh"

//#define RTL_APP_DEBUG
#if defined(RTL_APP_DEBUG)
#define PRINTK_APPD(fmt, ...)   printk("[DEBUG:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_APPD(fmt, ...)
#endif

#define CGOURP_PID_INIT   0
#define CGOURP_PID_STOP   9

#ifdef CONFIG_CMCC_JAVA_THREAD_CPU_USAGE
#define JAVA_THREAD_CPU_USAGE CONFIG_CMCC_JAVA_THREAD_CPU_USAGE
#else
#define JAVA_THREAD_CPU_USAGE 30
#endif

#ifdef CONFIG_CMCC_OSGI_TOTAL_CPU_USAGE
#define CMCC_OSGI_TOTAL_CPU_USAGE CONFIG_CMCC_OSGI_TOTAL_CPU_USAGE
#else
#define CMCC_OSGI_TOTAL_CPU_USAGE 90
#endif


#ifdef CONFIG_CMCC_OSGI_MEM_USAGE
#define OSGI_MEM_USAGE    CONFIG_CMCC_OSGI_MEM_USAGE
#else
#define OSGI_MEM_USAGE 256
#endif

#ifdef CONFIG_CMCC_OSGI_TOTAL_CPU_USAGE
#define CMCC_OSGI_TOTAL_CPU_USAGE CONFIG_CMCC_OSGI_TOTAL_CPU_USAGE
#else
#define CMCC_OSGI_TOTAL_CPU_USAGE 90
#endif

static DEFINE_MUTEX(cmcc_cgroup_mutex);
atomic_t cmcc_enable_value = ATOMIC_INIT(0);

static char group_path[PATH_MAX];
static const char appframework_path[8] = "/osgi";

static char memusage[8] = {0};
static char cpuusage[8] = {0};

typedef struct {
    struct work_struct my_work;
    pid_t pid;
    char pid_buff[8];
    char cpuusage_buff[8];
} my_work_t;

/******************************Functions **********************************/



static char *task_group_path1(struct task_struct *pt)
{
    struct task_group *tg = task_group(pt);
#if defined(CONFIG_SCHED_AUTOGROUP) || defined(CONFIG_SCHED_DEBUG)
    if (autogroup_path(tg, group_path, PATH_MAX))
        return group_path;
#endif
    return cgroup_path(tg->css.cgroup, group_path, PATH_MAX);
}

int rtk_cmcc_cgroup_path_check(struct task_struct *pt)
{
    if (strncmp
        (task_group_path1(pt), appframework_path,
         strlen(appframework_path)) == 0) {

        return 1;
    }
    return 0;
}

#if 0
int rtk_cmcc_cgroup_path_get(struct task_struct *pt, char *p_cgname)
{

    if (strncmp
        (task_group_path1(pt), appframework_path,
         strlen(appframework_path)) == 0) {
        //   printk("%s(%d)task_group_path1(pt) = %s\n", pt->comm, pt->pid, task_group_path1(pt));
        if (p_cgname != NULL) {
            snprintf(p_cgname, 32, "%s", task_group_path1(pt));
        }
        return 1;
    }
    return 0;
}
#endif
void rtk_cmcc_cpulimit_add(my_work_t *work);

static void rtk_cmcc_cpu_limit_init(void)
{
    char *argv[4];
	static char *envp[] = {
		"HOME=/",
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		NULL
	};
    int result;

    argv[0] = CMCC_LIMIT_INIT_SH;

	argv[1] = cpuusage;
    argv[2] = memusage;
    
    mutex_lock(&cmcc_cgroup_mutex);
    result = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

    PRINTK_APPD("[%s.%d]%s, totalcpu=%s, totalmem=%s,  result=%d\n", __func__, __LINE__, argv[0], argv[1], argv[2], result);
    mutex_unlock(&cmcc_cgroup_mutex);
    return;
}

static void rtk_cmcc_cpu_limit_stop(void)
{
    char *argv[4], *envp[3];
    int result;


    /* minimal command environment */
    envp[0] = "HOME=/";
    envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	envp[2] = NULL;

    argv[0] = "/bin/sh";
    argv[1] = CMCC_LIMIT_STOP_SH;
    argv[2] = NULL;
    argv[3] = NULL;
    
    mutex_lock(&cmcc_cgroup_mutex);
    result = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

    PRINTK_APPD("[%s.%d] result=%d\n", __func__, __LINE__, result);
    mutex_unlock(&cmcc_cgroup_mutex);
    return;
}

void rtk_cmcc_cpulimit_add(my_work_t *work)
{

    char *argv[4], *envp[3];
    int result;
    /* minimal command environment */
    envp[0] = "HOME=/";
    envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
    envp[2] = NULL;

    argv[0] = "/bin/sh";
    argv[1] = CMCC_LIMIT_SH;
    argv[2] = work->pid_buff;
    argv[3] = work->cpuusage_buff;

    mutex_lock(&cmcc_cgroup_mutex);
    result = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);

    PRINTK_APPD("[%s.%d] pid=%s, thread cpu=%s, result=%d\n", __func__, __LINE__,
                argv[2], argv[3], result);

    mutex_unlock(&cmcc_cgroup_mutex);

    return;
}

#if 0
void rtk_cmcc_cpulimit_del(char *p_cg_path, pid_t pid)
{
    char *argv[5], *envp[3];
    int result;
    char pid_buff[16];
    int umh_flag = UMH_KILLABLE;


    if (strcmp(p_cg_path, appframework_path) == 0) {
        return;
    }

    sprintf(pid_buff, "%d", pid);

    mutex_lock(&cmcc_cgroup_mutex);
    /* minimal command environment */
    envp[0] = "HOME=/";
    envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
    envp[2] = NULL;

    argv[0] = "/bin/sh";
    argv[1] = CMCC_LIMIT_SH;
    argv[2] = "del";
    argv[3] = pid_buff;
    argv[4] = p_cg_path;

    PRINTK_APPD("[%s]%s(%d) action=%s,del_pid=%s, p_cg_path=%s.\n",
                __func__, current->comm, current->pid, argv[2], argv[3],  argv[4]);

    result = call_usermodehelper(argv[0], argv, envp, umh_flag);
    schedule_timeout(1);


    mutex_unlock(&cmcc_cgroup_mutex);
    return;
}

#endif

/************************
 *
 *   work queue 
 ***********************/

static struct workqueue_struct *CmccWq = 0;

static void ework_handler(struct work_struct *pwork)
{
    my_work_t *temp;
    struct task_struct *task_tmp = NULL;
        /** pwork is the pointer of my_work **/
    temp = container_of(pwork, my_work_t, my_work);
    task_tmp = find_task_by_vpid(temp->pid);

    if (task_tmp) {
        rtk_cmcc_cpulimit_add(temp);
    } else {
        PRINTK_APPD("Not find pid(%d)\r\n", temp->pid);
    }
    kfree(temp);
}

void rtk_cmcc_add_work_q(pid_t pid)
{
    my_work_t *work = (my_work_t *) kmalloc(sizeof(my_work_t), GFP_KERNEL);
    if (work == NULL) {
        return;
    }
    work->pid = pid;
    
    sprintf(work->pid_buff     , "%d", pid);
    sprintf(work->cpuusage_buff, "%d", JAVA_THREAD_CPU_USAGE * NR_CPUS);

        /** Init the work struct with the work handler **/
    INIT_WORK(&(work->my_work), ework_handler);
    if (CmccWq) {
        queue_work(CmccWq, &(work->my_work));
    } else {
        kfree(work);
    }
}


/*
 * PROC File
 *  
 */

struct proc_dir_entry *cmcc_limit_proc_dir = NULL;
static int cmcc_limit_num_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, " cmcc_enable_value : %u\n",
               atomic_read(&cmcc_enable_value));
    return 0;
}

static int cmcc_limit_num_open(struct inode *inode, struct file *file)
{
    return single_open(file, cmcc_limit_num_show, inode->i_private);
}

static ssize_t cmcc_limit_num_write(struct file *file,
                                    const char __user * buf, size_t size,
                                    loff_t * _pos)
{

    unsigned char tmpBuf[16] = { 0 };
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len)) {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
        switch (input_val) {
            case 0:
                if (atomic_read(&cmcc_enable_value) == 1) {
                    atomic_set(&cmcc_enable_value, 0);
                    rtk_cmcc_cpu_limit_stop();
                }
                break;
            case 1:
                atomic_set(&cmcc_enable_value, 1);
                rtk_cmcc_cpu_limit_init();
                break;
            default:
                break;
        }
        return size;
    }
    return -EFAULT;
}


static const struct file_operations rtk_cmcc_limit_fops = {
    .owner = THIS_MODULE,
    .open = cmcc_limit_num_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
    .write = cmcc_limit_num_write,
};


int __init rtk_cmcc_limit_init(void)
{
    CmccWq = create_singlethread_workqueue("CMCC_CPULIMIT_Workqueue");
    if(CmccWq == NULL){
	  return -1;
	}
	
	sprintf(memusage, "%d", OSGI_MEM_USAGE);
	sprintf(cpuusage, "%d", CMCC_OSGI_TOTAL_CPU_USAGE * NR_CPUS);

    cmcc_limit_proc_dir = proc_mkdir(CMCC_LIMIT_PROC_DIR, NULL);
    if (cmcc_limit_proc_dir == NULL) {
        printk("create /proc/%s failed!\n", CMCC_LIMIT_PROC_DIR);
        return -1;
    }

    if (!proc_create
        (CMCC_LIMIT_PROC_FILE, 0444, cmcc_limit_proc_dir,
         &rtk_cmcc_limit_fops)) {
        printk("create proc %s/%s failed!\n", CMCC_LIMIT_PROC_DIR,
               CMCC_LIMIT_PROC_FILE);
        return -1;
    }


    return 0;
}

void __exit rtk_cmcc_limit_exit(void)
{
    remove_proc_entry(CMCC_LIMIT_PROC_FILE, cmcc_limit_proc_dir);
    proc_remove(cmcc_limit_proc_dir);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek CMCC CPULimit For Java Thread");
module_init(rtk_cmcc_limit_init);
module_exit(rtk_cmcc_limit_exit);
