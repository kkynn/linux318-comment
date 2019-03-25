/*
* ----------------------------------------------------------------
* Copyright c                  Realtek Semiconductor Corporation, 2016
* All rights reserved.
*
* Abstract: For YUEME APP Framework Protection
* ---------------------------------------------------------------
*/
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/mempolicy.h>
#include <linux/statfs.h>
#include <linux/cgroup.h>
#include "sched.h"

#undef RTL_APP_DEBUG
#if defined(RTL_APP_DEBUG)
#define PRINTK_APPD(fmt, ...)   printk("[DEBUG:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_APPD(fmt, ...)
#endif

static char group_path[PATH_MAX];
static const char appframework_path[16] = "/lxc/saf/lxc/";
char *task_group_path1(struct task_struct *pt)
{
    struct task_group *tg = task_group(pt);
#if defined(CONFIG_SCHED_AUTOGROUP) || defined(CONFIG_SCHED_DEBUG)
    if (autogroup_path(tg, group_path, PATH_MAX))
        return group_path;
#endif
    return cgroup_path(tg->css.cgroup, group_path, PATH_MAX);
}

int rtk_appframework_path_check(struct task_struct *pt)
{
    if (strncmp
        (task_group_path1(pt), appframework_path,
         strlen(appframework_path)) == 0) {
        PRINTK_APPD("task_group_path1(pt) = %s\n", task_group_path1(pt));
        return true;
    }
    return false;
}

static inline int rtk_appframework_child_check(struct task_struct *pt)
{
    struct task_struct *p, *n;
    int child_cnt = 0;
    read_lock(&tasklist_lock);
    list_for_each_entry_safe(p, n, &pt->children, sibling) {
        child_cnt++;
    }
    read_unlock(&tasklist_lock);
    return child_cnt;
}

#define RTL_APP_FORK_INHIBIT     1
#define RTL_APP_FORK_ALLOW       0
extern unsigned long app_limit;

int rtk_appframework_app_check(void)
{
    int child_cnt = 0;
    child_cnt = rtk_appframework_child_check(current);
    if (child_cnt >= app_limit) {
        if (rtk_appframework_path_check(current)) {
            PRINTK_APPD(KERN_WARNING
                        "\033[1;31m[%s] current process(%s, %d), parent(%s,%d),current children_cnt(%d)\033[0m\n",
                        __func__, current->comm, current->pid,
                        current->parent->comm, current->parent->pid, child_cnt);
            return RTL_APP_FORK_INHIBIT;
        }
    }
    return RTL_APP_FORK_ALLOW;
}

#define THREAD_LIMIT
#ifdef THREAD_LIMIT
extern unsigned long app_thread_limit;
int rtk_appframework_app_thread_limit(void)
{
    /*
     * thread_group_leader and child thread is reached app_limit
     * and the application is in cgroup "/lxc/saf/lxc/" for YUEME framework
     */
    if (thread_group_leader(current)
        && (current->signal->nr_threads > app_thread_limit)
        && rtk_appframework_path_check(current)) {
        printk(KERN_WARNING
               "\033[1;31mCurrent task(pid: %d, name: %s, Threads: %u ) reached to YUEME_APP_THREAD_LIMIT_NUM(%lu)!\033[0m\n",
               current->pid, current->comm, current->signal->nr_threads,
               app_thread_limit);
        return RTL_APP_FORK_INHIBIT;
    }
    return RTL_APP_FORK_ALLOW;
}
#else
int rtk_appframework_app_thread_limit(void)
{
    return RTL_APP_FORK_ALLOW;
}
#endif

#ifdef CONFIG_YUEME_FS_CHECK_FEATURE
#undef RTL_FSW_DEBUG
#if defined(RTL_FSW_DEBUG)
#define PRINTK_FSWD(fmt, ...)   printk("[FSW:(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)
#else
#define PRINTK_FSWD(fmt, ...)
#endif

#define RTL_APP_FS_W_INHIBIT     1
#define RTL_APP_FS_W_ALLOW       0
#define RTL_APP_FS_PERCNT      CONFIG_YUEME_FS_LIMIT_PERCENTAGE
unsigned long yueme_appf_fs_check(struct file *file, loff_t * pos,
                                  size_t * count)
{
    int ret;
    long appf_fs_avail;
    long appf_fs_usead;
    struct kstatfs stats;

    /* If the process is not in cgroup ""/lxc/saf/lxc/""
     * We don't limit the quota
     */

    if (rtk_appframework_path_check(current) == 0) {
       return RTL_APP_FS_W_ALLOW;
    }
	   
    ret = vfs_statfs(&file->f_path, &stats);
    if (ret == 0) {
        appf_fs_usead = stats.f_blocks - stats.f_bavail;
        appf_fs_avail = (long)(stats.f_blocks * RTL_APP_FS_PERCNT) / 100;

        if (appf_fs_usead >= appf_fs_avail) {
            PRINTK_FSWD(" appf_fs_usead = %ld, appf_fs_avail = %ld \n",
                        appf_fs_usead, appf_fs_avail);
            PRINTK_FSWD(" %ld, %ld, %llu, %llu, %llu, %llu, %llu \n",
                        stats.f_type, stats.f_bsize, stats.f_blocks,
                        stats.f_bfree, stats.f_bavail, stats.f_files,
                        stats.f_ffree);
            PRINTK_FSWD("FS(yaffs) usage is higher than %d percent.\n",
                        RTL_APP_FS_PERCNT);
            return RTL_APP_FS_W_INHIBIT;
        }

        if (((*count / stats.f_bsize) + appf_fs_usead) >= appf_fs_avail) {
            PRINTK_FSWD(" appf_fs_usead = %ld, appf_fs_avail = %ld \n",
                        appf_fs_usead, appf_fs_avail);
            PRINTK_FSWD(" %ld, %ld, %llu, %llu, %llu, %llu, %llu \n",
                        stats.f_type, stats.f_bsize, stats.f_blocks,
                        stats.f_bfree, stats.f_bavail, stats.f_files,
                        stats.f_ffree);
            if (printk_ratelimit())
                pr_warning
                    ("This write(%d Byte) will cause FS(yaffs) usage is higher than %d percent.\n",
                     *count, RTL_APP_FS_PERCNT);
            return RTL_APP_FS_W_INHIBIT;
        }

    } else {
        PRINTK_FSWD("[%s.%d]ret =%d\n", __func__, __LINE__, ret);
    }

    return RTL_APP_FS_W_ALLOW;
}
#endif
