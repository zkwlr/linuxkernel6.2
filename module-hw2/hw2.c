#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define PROC_NAME "hw2"

MODULE_AUTHOR("Ha, Donghyun");
MODULE_LICENSE("GPL");

struct task_info {
    bool is_info_exist; // 정보가 저장됐는지 확인하는 flag, false로 기본 설정
    char name[TASK_COMM_LEN];
    pid_t pid;
    unsigned long long up_time;
    unsigned long long start_time;
    unsigned long flags;
};
struct task_info task_info_list[5];
int task_index = 0;

static struct timer_list my_timer;

void my_tasklet_function(unsigned long data) {

    // printk(KERN_INFO "my_tasklet_function was called.\n");

    //현재 task 정보를 담을 구조체 변수 선언해 정보 저장 후 task_info_list에 저장
    struct task_info latest_task_info;
    struct task_struct *task;
    struct task_struct *latest_task;
    unsigned long long latest_start_time = 0;

    // 가장 최근에 실행된 task를 찾아 latest_task에 저장 후 정보를 찾는다.
    for_each_process(task) {
        // 탐색하는 task의 flags가 커널 스레드 flag가 아니고, start_time이 현재 latest_start_time보다 크면
        if (!(task->flags & PF_KTHREAD) & (task->start_time > latest_start_time)) {
            latest_start_time = task->start_time;
            latest_task = task;
        }
    }
    // latest_task의 정보를 latest_task_info에 저장
    latest_task_info.pid = latest_task->pid;
    strcpy(latest_task_info.name, latest_task->comm);
    latest_task_info.up_time = ktime_get_ns() / 1000000000; // 명령 실행된 시각
    latest_task_info.start_time = latest_start_time / 1000000000;
    latest_task_info.flags = latest_task->flags;

     // latest_start_time이 0이면 유효한 정보가 아니므로 flag를 false로 설정
    if (latest_start_time != 0) {
        latest_task_info.is_info_exist = true; // 정보가 저장됐음을 flag에 기록
    }

    // task_info_list에 최근 정보가 마지막 index로 가게 저장
    if (task_index < 5) { // 배열이 아직 차기 전이면
        task_info_list[task_index] = latest_task_info;
        task_index++;
    }
    else { // 배열이 가득 차 있으면
        for (int i = 0; i < 4; i++) {
            task_info_list[i] = task_info_list[i + 1]; // 앞으로 한칸씩 이동
        }
        task_info_list[4] = latest_task_info; // 마지막 index에 latest_task_info 저장
    }
}

DECLARE_TASKLET_OLD(my_tasklet, my_tasklet_function);

// timer_list *을 인수로 넘기는 callback 함수를 만들어 타이머가 만료되면 실행될수 있게 함
// tasklet 실행 후 타이머를 10초 뒤로 다시 설정해 callback 함수를 10초 후 다시 실행
static void callback_timer(struct timer_list *timer) {
    tasklet_schedule(&my_tasklet);
    mod_timer(timer, jiffies + msecs_to_jiffies(10000));
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
    // 기본 정보 출력 - 학번, 이름, 현재 시스템의 Uptime (초)
    seq_printf(s, "[System Programming Assignment #2]\n");
    seq_printf(s, "ID: 2019147542\n");
    seq_printf(s, "Name: Ha, Donghyun\n");
    seq_printf(s, "Uptime (s): %llu\n", ktime_get_ns() / 1000000000);
    seq_printf(s, "--------------------------------------------------\n");

    // 가장 마지막 기록 5번 출력 (5번 미만으로 정보가 수집된 경우 수집된 횟수만큼만 출력)

    for (int i = 0; i < 5; i++) { // 배열 탐색
        if (task_info_list[i].is_info_exist) { // 정보가 있으면 출력
            seq_printf(s, "[Trace #%d]\n", i);
            seq_printf(s, "Uptime (s): %llu\n", task_info_list[i].up_time);
            seq_printf(s, "Command: %s\n", task_info_list[i].name);
            seq_printf(s, "PID: %d\n", task_info_list[i].pid);
            seq_printf(s, "Start time (s): %llu\n", task_info_list[i].start_time);
            seq_printf(s, "PGD base address: 0x%lx\n", 0);
            seq_printf(s, "Flags: 0x%lx\n", task_info_list[i].flags);
            // Code Area
            seq_printf(s, "Code Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", 0); // unsigned long, 16진수로 출력
            seq_printf(s, "- start (PGD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- start (PUD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- start (PMD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- start (PTE): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- start (physical): 0x%lx\n", 0);
            seq_printf(s, "- end (virtual): 0x%lx\n", 0);
            seq_printf(s, "- end (PGD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- end (PUD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- end (PMD): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- end (PTE): 0x%lx, 0x%lx\n", 0, 0);
            seq_printf(s, "- end (physical): 0x%lx\n", 0);
            // Data Area
            seq_printf(s, "Data Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", 0);
            seq_printf(s, "- start (physical): 0x%lx\n", 0);
            seq_printf(s, "- end (virtual): 0x%lx\n", 0);
            seq_printf(s, "- end (physical): 0x%lx\n", 0);
            // Heap Area
            seq_printf(s, "Heap Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", 0);
            seq_printf(s, "- start (physical): 0x%lx\n", 0);
            seq_printf(s, "- end (virtual): 0x%lx\n", 0);
            seq_printf(s, "- end (physical): 0x%lx\n", 0);
            // Stack Area
            seq_printf(s, "Stack Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", 0);
            seq_printf(s, "- start (physical): 0x%lx\n", 0);
            seq_printf(s, "- end (virtual): 0x%lx\n", 0);
            seq_printf(s, "- end (physical): 0x%lx\n", 0);
            seq_printf(s, "--------------------------------------------------\n");
        }
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

    // 첫번째 tasklet 실행
    tasklet_schedule(&my_tasklet);
    // my timer 초기화(시간이 되면 callback_timer 통해 tasklet_schedule 함수 호출되게 my timer 설정)
    timer_setup(&my_timer, callback_timer, 0);
    // 10초 후에 callback_timer 함수 호출해 tasklet_schedule 함수 호출
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(10000)); 

    return 0; 
}

static void __exit hello_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    del_timer_sync(&my_timer);
    tasklet_kill(&my_tasklet);
}

module_init(hello_init);
module_exit(hello_exit);
