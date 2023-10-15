#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/jiffies.h>

#define PROC_NAME "hw1"

MODULE_AUTHOR("Ha, Donghyun");
MODULE_LICENSE("GPL");

//core.c에 있는 schedule_info_list를 가져와 사용
extern struct schedule_info schedule_info_list[20];

static void *hello_seq_start(struct seq_file *s, loff_t *pos)
{
    static unsigned long counter = 0;
    if (*pos == 0)
        return &counter;
    else {
        *pos = 0;
        return NULL;
    }
}

static void *hello_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    unsigned long *tmp_v = (unsigned long *)v;
    (*tmp_v)++;
    (*pos)++;
    return NULL;
}

static void hello_seq_stop(struct seq_file *s, void *v)
{
    // nothing to do
}

static int hello_seq_show(struct seq_file *s, void *v)
{
    loff_t *spos = (loff_t *) v;
    int num_cpus = num_online_cpus();
    seq_printf(s, "[System Programming Assignment #1]\n");
    seq_printf(s, "ID: 2019147542\n");
    seq_printf(s, "Name: Ha, Donghyun\n");
    seq_printf(s, "CPU: %d\n", num_cpus);
    seq_printf(s, "--------------------------------------------------\n");
    for (int i = 0; i < 20; i++) {
        seq_printf(s, "schedule() trace #%d - CPU ", i);
        seq_printf(s,"#%d\n", schedule_info_list[i].cpu);
        seq_printf(s, "Command: %s\n", schedule_info_list[i].task_name1);
        seq_printf(s, "PID: %d\n", schedule_info_list[i].pid);
        seq_printf(s, "Priority: %d\n", schedule_info_list[i].prio);
        seq_printf(s, "Start time (ms): %lu\n", schedule_info_list[i].runtime);
        seq_printf(s, "Scheduler: \n");
        seq_printf(s, "->\n");
        seq_printf(s, "Command: %s\n", schedule_info_list[i].ntask_name1);
        seq_printf(s, "PID: %d\n", schedule_info_list[i].npid);
        seq_printf(s, "Priority: %d\n", schedule_info_list[i].nprio);
        seq_printf(s, "Start time (ms): %lu\n", schedule_info_list[i].nruntime);
        seq_printf(s, "Scheduler: \n");
        seq_printf(s, "--------------------------------------------------\n");
    }
    
    struct task_struct *p;
        p = get_current();   // 현재 프로세스를 얻는다

        // 자식과 부모의 pid, state, comm 을 얻는다
        seq_printf(s, "[Kernel Message] : Currnet Process\n");
        seq_printf(s, "pid = %d\n", p->pid); 
        seq_printf(s, "comm = %s\n", p->comm);
        seq_printf(s, "priority = %d\n", p->prio);  
        struct task_struct *pa;
        pa = p->real_parent;   // 현재 프로세스가 가르키는 부모 프로세스를 얻는다

        seq_printf(s, "[Kernel Message] : Parent Process\n");
        seq_printf(s, "pid = %d\n", pa->pid); 
        seq_printf(s, "comm = %s\n", pa->comm);
        seq_printf(s, "priority = %d\n", pa->prio);   

    return 0;
}

static struct seq_operations hello_seq_ops = {
    .start = hello_seq_start,
    .next = hello_seq_next,
    .stop = hello_seq_stop,
    .show = hello_seq_show
};

static int hello_proc_open(struct inode *inode, struct file *file) {
    return seq_open(file, &hello_seq_ops);
}

static const struct proc_ops hello_proc_ops = {
    .proc_open = hello_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release
};

static int __init hello_init(void) {
    struct proc_dir_entry *proc_file_entry;
    proc_file_entry = proc_create(PROC_NAME, 0, NULL, &hello_proc_ops);
    return 0; 
}

static void __exit hello_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
}

module_init(hello_init);
module_exit(hello_exit);
