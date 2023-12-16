#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/interrupt.h>

#define PROC_NAME "hw2"

MODULE_AUTHOR("Ha, Donghyun");
MODULE_LICENSE("GPL");

struct task_info {
    pid_t pid;
    char name[TASK_COMM_LEN];
    long state;
    unsigned long long up_time;
    unsigned long long start_time;
};
struct task_info task_info_list[10];

static struct tasklet_struct my_tasklet;
void my_tasklet_function(unsigned long data) {
    struct task_struct *task = current;
    task_info_list[0].pid = task->pid;
    strcpy(task_info_list[0].name, task->comm);
    task_info_list[0].up_time = ktime_get_ns() / 1000000000;
    task_info_list[0].start_time = task->start_time / 1000000000;
    task_info_list[1].pid = 1;
    
}

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
    seq_printf(s, "Hello World!\n");
    for (int i = 0; i < 10; i++) {
        seq_printf(s, "PID: %d\n", task_info_list[i].pid);
        seq_printf(s, "Name: %s\n", task_info_list[i].name);
        seq_printf(s, "Uptime (s): %llu\n", task_info_list[i].up_time);
        seq_printf(s, "Start time (s): %llu\n", task_info_list[i].start_time);
        seq_printf(s, "--------------------------------------------------\n");
    }
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
    tasklet_init(&my_tasklet, my_tasklet_function, 0);
    tasklet_schedule(&my_tasklet);
    return 0; 
}

static void __exit hello_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    tasklet_kill(&my_tasklet);
}

module_init(hello_init);
module_exit(hello_exit);
