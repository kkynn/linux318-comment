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
 * Purpose : Limits fork number for appframewrk
 * 
 * Feature :
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
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/cgroup.h>
#include <linux/kthread.h>
#include <linux/hashtable.h>
#include <linux/delay.h>
#define APP_M_HASH_BITS       9
/*Internal switch option*/
#define THREAD_LIMIT 1

extern char *task_group_path1(struct task_struct *pt);

#define  MIN_PROCESS_CNT          (32)  //essential processes

unsigned long app_limit = CONFIG_YUEME_APP_FORK_LIMIT_NUM;
#ifdef THREAD_LIMIT
unsigned long app_thread_limit = CONFIG_YUEME_APP_THREAD_LIMIT_NUM;
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER


static DEFINE_HASHTABLE(app_m_table, APP_M_HASH_BITS);
struct app_m{
  struct hlist_node hlist;
  pid_t ppid;
  int count;
  char comm[16];
};

static char appframework_path[32] = "/lxc/saf/lxc/";

int app_limit_seconds = 120;
struct task_struct *appframe_monitor_task;


static int app_m_hash(int pid){
 return ( pid % 512);
}

static struct app_m *appm_hash_search(pid_t pid){
      int key = app_m_hash(pid);
      struct app_m *app_m_p;   
         hash_for_each_possible(app_m_table, app_m_p, hlist, key) {
                 if (app_m_p->ppid != pid)
                          continue; 
                 /* This css_set matches what we need */
                 return app_m_p;
        };
	return NULL;
}


static void appm_hash_add(struct task_struct *p){
  struct app_m *app_m_p ;
  
  app_m_p= kzalloc(sizeof(*app_m_p), GFP_KERNEL);
  if (!app_m_p)
        return;
  
  INIT_HLIST_NODE(&app_m_p->hlist);
  memcpy(app_m_p->comm, p->comm, 16);
  app_m_p->ppid = p->parent->pid;
  app_m_p->count = 1;
  hash_add(app_m_table, &app_m_p->hlist, app_m_hash(app_m_p->ppid));
}

static void appm_hash_clear(void){
      int num;
      struct app_m *app_m_p;   
      hash_for_each(app_m_table, num, app_m_p, hlist) {
              hash_del(&app_m_p->hlist);
	      kfree(app_m_p);
      };
      return;
}


 /*
  * The process p may have detached its own ->mm while exiting or through
  * use_mm(), but one or more of its subthreads may still have a valid
  * pointer.  Return p, or any of its subthreads with a valid ->mm, with
  * task_lock() held.
  */
#if 0
 static struct task_struct *find_lock_task_mm(struct task_struct *p)
 {
         struct task_struct *t;
 
         rcu_read_lock();
 
         for_each_thread(p, t) {
                 task_lock(t);
                 if (likely(t->mm))
                         goto found;
                 task_unlock(t);
         }
         t = NULL;
 found:
         rcu_read_unlock();
 
         return t;
}
#endif
extern  long sys_kill(int pid, int sig);
static void appframe_kill_process(struct task_struct *selectedp){
   int total_process = 0;

   struct task_struct *p;
   struct task_struct *parent;
   struct task_struct *pparent;
   int parent_pid = 0;
   int kill_cnt = 0;
   printk("[appframework_monitor]vitim:%s (%d) ; parent:%s (%d)\n", selectedp->comm, selectedp->pid, selectedp->parent->comm, selectedp->parent->pid );
   parent = selectedp->parent;
   pparent = parent->parent;
	rcu_read_lock();
	parent_pid = selectedp->parent->pid;
	for_each_process(p){
		if (p->parent->pid == selectedp->parent->pid) {
#if 0
			task_lock(p);	/* Protect ->comm from prctl() */
			printk_once("Kill process %d (%s) sharing parent id\n", task_pid_nr(p), p->comm);
			task_unlock(p);
#endif
			kill_cnt++;
 			do_send_sig_info(SIGKILL, SEND_SIG_FORCED, p, true);
		}else{
		  	total_process++;
		}
	}
	rcu_read_unlock();
	if(parent){
	    printk("[appframework_monitor] Kill process with parent pid:%d(%s). %d Childs\n", parent->pid, parent->comm, kill_cnt);
             do_send_sig_info(SIGKILL, SEND_SIG_FORCED, parent, true);
	     total_process--;
	     if(pparent->pid > 63){
	       	printk("[appframework_monitor]Kill pparent process pid:%d(%s).\n", pparent->pid, pparent->comm);
	        do_send_sig_info(SIGKILL, SEND_SIG_FORCED, pparent, true);
	        total_process--;
	     }
	}  
}

static void appframework_monitor_main(void){
  	struct task_struct *g, *p;
	struct app_m *app_m_p;
	struct task_struct *viticm_p = NULL;
	int viticm_count = 0;
	
	if(atomic_read(&current->real_cred->user->processes) < (app_limit + MIN_PROCESS_CNT)){
#ifdef DEBUG_APPF_KILLER
 	  printk("[appframework_monitor] Total_Process=%d. < (%lu).Nothing to do.\n", 
		 atomic_read(&current->real_cred->user->processes) , (app_limit + MIN_PROCESS_CNT));
#endif
	  /* Nothing to */
	  return;
	}
	
	
	rcu_read_lock();
	for_each_process_thread(g, p) {
            
	  if( strncmp(task_group_path1(p) , appframework_path, strlen(appframework_path)) == 0){
	    
// 	    printk("name:%s, pid:%d, %s -- parent_pid:%d , parent:%s\n", p->comm, p->pid,  task_group_path1(p) , p->parent->pid, p->parent->comm);
	    app_m_p = appm_hash_search(p->parent->pid);
	    if(app_m_p){
	      app_m_p->count = app_m_p->count + 1;
// 	      	    printk("     parent_pid:%d , parent:%s, count:%d\n", app_m_p->ppid, app_m_p->comm,app_m_p->count);
		    
		    if((app_m_p->count >= app_limit) && (app_m_p->count > viticm_count) ){
		      viticm_p = p;
		      viticm_count = app_m_p->count;
		    }
		    
		    
	    }else{
	      appm_hash_add(p);
	    }
	    
	  }
	  
	  
	}
	
	appm_hash_clear();
	
	rcu_read_unlock();
	
	if(viticm_count){
	   printk("[appframework_monitor] kill  pid:%d ,%s, count:%d\n", viticm_p->pid, viticm_p->comm, viticm_count);
	   appframe_kill_process(viticm_p);
	}
	
        return;
}

static  int appframework_monitor(void *arg)
{
	while (!kthread_should_stop()) {
		appframework_monitor_main();
           	set_current_state(TASK_INTERRUPTIBLE);
		/* Now sleep */
		schedule_timeout(app_limit_seconds*HZ);
	}

	return 0;
}



static void appframework_app_monitor(void){
 
		appframe_monitor_task = kthread_create(appframework_monitor, NULL, "appf_m");
		if (WARN_ON(!appframe_monitor_task)) {
			printk("appframework_app_monitor: create appframework_monitor failed!");
			goto out_free;
		}

 		wake_up_process(appframe_monitor_task);
out_free:
       return;

}

#endif

static int app_limit_num_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, " app_limit : %lu\n", app_limit );
    return 0;
}

static ssize_t app_limit_num_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
       if(input_val > 0){
           app_limit = input_val;
       }else{
          printk("Process limit num <= 0 \n");
       }
        return size;
    }
    return -EFAULT;
}


struct proc_dir_entry *yueme_app_proc_dir = NULL;

static int app_limit_num_open(struct inode *inode, struct file *file)
{
    return single_open(file, app_limit_num_show, inode->i_private);
}
static const struct file_operations bsp_luna_test_fops = {
    .owner      = THIS_MODULE,
    .open       = app_limit_num_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = app_limit_num_write,
};

/* killer */
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
static int app_killertime_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, " appframework_app_monitor period time : %d (s)\n", app_limit_seconds );
    return 0;
}

static ssize_t app_killertime_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
       if((input_val > 0) && (input_val < 86400)){ //between 1s and 24 hours
           app_limit_seconds = input_val;
       }else if(input_val == 0){
	 if(appframe_monitor_task)
	    kthread_stop(appframe_monitor_task);
	 
	  printk("Stop kthread[appf_m] \n");
	  appframe_monitor_task = NULL;
       }
        return size;
    }
    return -EFAULT;
}



static int app_killertime_open(struct inode *inode, struct file *file)
{
    return single_open(file, app_killertime_show, inode->i_private);
}
static const struct file_operations bsp_luna_killer_fops = {
    .owner      = THIS_MODULE,
    .open       = app_killertime_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = app_killertime_write,
};

#endif

#ifdef THREAD_LIMIT
static int thread_limit_num_show(struct seq_file *seq, void *v)
{
    seq_printf(seq, " app thread_limit : %lu\n", app_thread_limit );
    return 0;
}
static ssize_t thread_limit_num_write(struct file *file,  const char __user *buf, size_t size, loff_t *_pos)
{

    unsigned char tmpBuf[16] = {0};
    int len = (size > 15) ? 15 : size;
    int input_val = 0;
    if (buf && !copy_from_user(tmpBuf, buf, len))
    {
        input_val = simple_strtoul(tmpBuf, NULL, 0);
       if(input_val > 0){
           app_thread_limit = input_val;
       }else{
          printk("Process limit num <= 0 \n");
       }
        return size;
    }
    return -EFAULT;
}
static int thread_limit_num_open(struct inode *inode, struct file *file)
{
    return single_open(file, thread_limit_num_show, inode->i_private);
}
static const struct file_operations bsp_luna_threadlimit_fops = {
    .owner      = THIS_MODULE,
    .open       = thread_limit_num_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
    .write      = thread_limit_num_write,
};
#endif

#define YUEME_APPLIMIT_PROC_DIR    "luna_yueme_applimit"
int __init luna_yueme_app_init(void)
{

    yueme_app_proc_dir = proc_mkdir(YUEME_APPLIMIT_PROC_DIR, NULL);
    if (yueme_app_proc_dir == NULL) {
        printk("create /proc/%s failed!\n", YUEME_APPLIMIT_PROC_DIR);
        return 1;
    }

    if(! proc_create("AppFramework_applimit", 0444, yueme_app_proc_dir, &bsp_luna_test_fops)){
      printk("create proc %s/AppFramework_applimit failed!\n", YUEME_APPLIMIT_PROC_DIR);
      return 1;
    }
#ifdef THREAD_LIMIT
	if(! proc_create("AppFramework_threadlimit", 0444, yueme_app_proc_dir, &bsp_luna_threadlimit_fops)){
      printk("create proc %s/AppFramework_threadlimit failed!\n", YUEME_APPLIMIT_PROC_DIR);
      return 1;
    }
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
    if(! proc_create("appf_monitor_time", 0444, yueme_app_proc_dir, &bsp_luna_killer_fops)){
      printk("create proc %s/AppFramework_killer_time failed!\n", YUEME_APPLIMIT_PROC_DIR);
      return 1;
    }
#endif
#ifdef CONFIG_YUEME_APP_FORK_LIMIT_KILLER
    appframework_app_monitor();
#endif
    
    return 0;
}

void __exit luna_yueme_app_exit(void)
{
     remove_proc_entry("AppFramework_applimit", yueme_app_proc_dir);
     proc_remove(yueme_app_proc_dir);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek YUEME APP Limit : ");
module_init(luna_yueme_app_init);
module_exit(luna_yueme_app_exit);
