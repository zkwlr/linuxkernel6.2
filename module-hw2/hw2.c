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
#include <linux/pgtable.h>

#define PROC_NAME "hw2"

MODULE_AUTHOR("Ha, Donghyun");
MODULE_LICENSE("GPL");

struct task_info {
    bool is_info_exist; // 정보가 저장됐는지 확인하는 flag, false로 기본 설정
    char name[TASK_COMM_LEN];
    pid_t pid;
    unsigned long long up_time;
    unsigned long long start_time;
    // unsigned long flags;
    // Page grobal directory의 시작 주소(base address)를 저장하는 포인터 변수(포인터면서 그 자체로 우리가 원하는 주소값)
    pgd_t *pgd; // PGD base address
    // Code Area의 start, end 주소를 저장하는 포인터 변수
    unsigned long code_start_vaddr, code_end_vaddr;
    pgd_t *code_start_pgd, *code_end_pgd;
    p4d_t *code_start_p4d, *code_end_p4d;
    pud_t *code_start_pud, *code_end_pud;
    pmd_t *code_start_pmd, *code_end_pmd;
    pte_t *code_start_pte, *code_end_pte;
    // 출력용 val 저장 변수 - 포인터 값은 계속 변하므로 정보를 scan하는 시점의 값을 저장해서 출력해야 한다.
    // 바로 val 함수에 포인터로 넘겨 출력하면 그 시점의 변화한 값이 출력되므로
    unsigned long start_pgd_val, end_pgd_val;
    unsigned long start_pud_val, end_pud_val;
    unsigned long start_pmd_val, end_pmd_val;
    unsigned long start_pte_val, end_pte_val;
    unsigned long code_start_paddr, code_end_paddr;
    // unsigned long code_start_paddr1;
    // Data Area의 가상, 물리 start, end 주소를 저장하는 포인터 변수
    unsigned long data_start_vaddr, data_end_vaddr;
    unsigned long data_start_paddr, data_end_paddr;
    // Heap Area의 가상, 물리 start, end 주소를 저장하는 포인터 변수
    unsigned long heap_start_vaddr, heap_end_vaddr;
    unsigned long heap_start_paddr, heap_end_paddr;
    // Stack Area의 가상, 물리 start, end 주소를 저장하는 포인터 변수
    unsigned long stack_start_vaddr, stack_end_vaddr;
    unsigned long stack_start_paddr, stack_end_paddr;
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
        // 탐색하는 task의 flag의 PF_KTHREAD field가 0010이면 kthread이므로
        // flags와 0x00200000을 bitwise and 연산해 kthread field를 확인한다.
        // 결과 0이면 flags의 PF_KTHREAD field가 0이므로 결과가 0이 나오므로 if문을 실행한다.(kthread가 아님)
        if (!(task->flags & PF_KTHREAD)) {
            // start_time이 현재 latest_start_time보다 크면
            if ((task->start_time > latest_start_time)) {
                latest_start_time = task->start_time;
                latest_task = task;
            }
        }
    }
    // latest_task의 정보를 latest_task_info에 저장
    latest_task_info.pid = latest_task->pid;
    strcpy(latest_task_info.name, latest_task->comm);
    latest_task_info.up_time = ktime_get_ns() / 1000000000; // 명령 실행된 시각
    latest_task_info.start_time = latest_start_time / 1000000000;
    // latest_task_info.flags = latest_task->flags;

    latest_task_info.pgd = latest_task->mm->pgd; // Page global directory의 시작 주소(포인터) 저장

    // Code Area의 시작부분의 가상주소 저장, task의 mm과 이 시작 가상주소를 이용해 pgd, pud 등등의 시작 주소를 찾는다.
    latest_task_info.code_start_vaddr = latest_task->mm->start_code;
    // Code Area의 시작부분의 가상주소를 이용해 pgd, pud, pmd, pte의 시작 주소를 찾는다.
    // 각각의 offset 함수들은 가상 주소의 자신 part의 offset+ 테이블의 주소를 계산해 다음 테이블에서 가리키는 값을 계산한다.
    // pgd_val(*pgd)는 pgd가 가리키는 주소의 값(pgd_t의 주소가 아니라 pgd_t에 저장된 값(pud의 주소))을 가져온다.
    // 즉 다음 level인 pud의 주소를 가져온다. 
    latest_task_info.code_start_pgd = pgd_offset(latest_task->mm, latest_task_info.code_start_vaddr);
    latest_task_info.start_pgd_val = pgd_val(*latest_task_info.code_start_pgd);
    // 강의안과 다르게 pgd와 pud 사이에 p4d가 있어서 p4d_offset 함수를 추가해야한다.(val은 저장 필요 X)
    latest_task_info.code_start_p4d = p4d_offset(latest_task_info.code_start_pgd, latest_task_info.code_start_vaddr);
    latest_task_info.code_start_pud = pud_offset(latest_task_info.code_start_p4d, latest_task_info.code_start_vaddr);
    latest_task_info.start_pud_val = pud_val(*latest_task_info.code_start_pud);
    latest_task_info.code_start_pmd = pmd_offset(latest_task_info.code_start_pud, latest_task_info.code_start_vaddr);
    latest_task_info.start_pmd_val = pmd_val(*latest_task_info.code_start_pmd);
    latest_task_info.code_start_pte = pte_offset_kernel(latest_task_info.code_start_pmd, latest_task_info.code_start_vaddr);
    latest_task_info.start_pte_val = pte_val(*latest_task_info.code_start_pte);
    // Code Area의 시작부분의 가상주소와 테이블을 따라 구한 pte의 값을 이용해 물리주소를 계산한다.
    
    // pte의 값과 PAGE_MASK(0xfffff000)을 bitwise and 연산해 pte의 값의 뒤 12bit(offset 부분)를 masking해 0으로 만든다.(2^12byte=4KB 크기의 page frame 번호가 된다.)
    // vaddr은 ~PAGE_MASK(0x00000fff)과 bitwise and 연산해 뒤 12자리의 값(offset)만 남기고 지운다.
    // 그 후 pte의 앞 20bit와 vaddr의 offset 부분 12bit를 더해 물리주소를 계산한다.
    latest_task_info.code_start_paddr = (pte_val(*latest_task_info.code_start_pte) & PAGE_MASK) + (latest_task_info.code_start_vaddr & ~PAGE_MASK);
    // latest_task_info.code_start_paddr1 = pte_val(*latest_task_info.code_start_pte) & PTE_PFN_MASK;
    // 다른 방식: pte_pfn()으로 pte의 값을 page frame number로 바꾼 후
    // PAGE_SHIFT(12)만큼 left shift 해서 뒷 12bit를 남긴다.(위의 pte_val()을 mask하는것과 같은 방식)
    // latest_task_info.code_start_paddr1 = (pte_pfn(*latest_task_info.code_start_pte) << PAGE_SHIFT) + (latest_task_info.code_start_vaddr & ~PAGE_MASK);
    
    // Code Area의 끝부분의 가상주소 저장
    latest_task_info.code_end_vaddr = latest_task->mm->end_code;
    latest_task_info.code_end_pgd = pgd_offset(latest_task->mm, latest_task_info.code_end_vaddr);
    latest_task_info.end_pgd_val = pgd_val(*latest_task_info.code_end_pgd);
    latest_task_info.code_end_p4d = p4d_offset(latest_task_info.code_end_pgd, latest_task_info.code_end_vaddr);
    latest_task_info.code_end_pud = pud_offset(latest_task_info.code_end_p4d, latest_task_info.code_end_vaddr);
    latest_task_info.end_pud_val = pud_val(*latest_task_info.code_end_pud);
    latest_task_info.code_end_pmd = pmd_offset(latest_task_info.code_end_pud, latest_task_info.code_end_vaddr);
    latest_task_info.end_pmd_val = pmd_val(*latest_task_info.code_end_pmd);
    latest_task_info.code_end_pte = pte_offset_kernel(latest_task_info.code_end_pmd, latest_task_info.code_end_vaddr);
    latest_task_info.end_pte_val = pte_val(*latest_task_info.code_end_pte);
    // 물리주소 계산
    latest_task_info.code_end_paddr = (pte_val(*latest_task_info.code_end_pte) & PAGE_MASK) + (latest_task_info.code_end_vaddr & ~PAGE_MASK);
    //latest_task_info.code_end_paddr = pte_val(*latest_task_info.code_end_pte) & PTE_PFN_MASK;

    // Data Area의 가상주소, 물리주소
    latest_task_info.data_start_vaddr = latest_task->mm->start_data;
    pgd_t *data_start_pgd = pgd_offset(latest_task->mm, latest_task_info.data_start_vaddr);
    p4d_t *data_start_p4d = p4d_offset(data_start_pgd, latest_task_info.data_start_vaddr);
    pud_t *data_start_pud = pud_offset(data_start_p4d, latest_task_info.data_start_vaddr);
    pmd_t *data_start_pmd = pmd_offset(data_start_pud, latest_task_info.data_start_vaddr);
    pte_t *data_start_pte = pte_offset_kernel(data_start_pmd, latest_task_info.data_start_vaddr);
    latest_task_info.data_start_paddr = (pte_val(*data_start_pte) & PAGE_MASK) + (latest_task_info.data_start_vaddr & ~PAGE_MASK);
    latest_task_info.data_end_vaddr = latest_task->mm->end_data;
    pgd_t *data_end_pgd = pgd_offset(latest_task->mm, latest_task_info.data_end_vaddr);
    p4d_t *data_end_p4d = p4d_offset(data_end_pgd, latest_task_info.data_end_vaddr);
    pud_t *data_end_pud = pud_offset(data_end_p4d, latest_task_info.data_end_vaddr);
    pmd_t *data_end_pmd = pmd_offset(data_end_pud, latest_task_info.data_end_vaddr);
    pte_t *data_end_pte = pte_offset_kernel(data_end_pmd, latest_task_info.data_end_vaddr);
    latest_task_info.data_end_paddr = (pte_val(*data_end_pte) & PAGE_MASK) + (latest_task_info.data_end_vaddr & ~PAGE_MASK);
    // Heap Area의 가상주소, 물리주소
    latest_task_info.heap_start_vaddr = latest_task->mm->start_brk;
    pgd_t *heap_start_pgd = pgd_offset(latest_task->mm, latest_task_info.heap_start_vaddr);
    p4d_t *heap_start_p4d = p4d_offset(heap_start_pgd, latest_task_info.heap_start_vaddr);
    pud_t *heap_start_pud = pud_offset(heap_start_p4d, latest_task_info.heap_start_vaddr);
    pmd_t *heap_start_pmd = pmd_offset(heap_start_pud, latest_task_info.heap_start_vaddr);
    pte_t *heap_start_pte = pte_offset_kernel(heap_start_pmd, latest_task_info.heap_start_vaddr);
    latest_task_info.heap_start_paddr = (pte_val(*heap_start_pte) & PAGE_MASK) + (latest_task_info.heap_start_vaddr & ~PAGE_MASK);
    latest_task_info.heap_end_vaddr = latest_task->mm->brk;
    pgd_t *heap_end_pgd = pgd_offset(latest_task->mm, latest_task_info.heap_end_vaddr);
    p4d_t *heap_end_p4d = p4d_offset(heap_end_pgd, latest_task_info.heap_end_vaddr);
    pud_t *heap_end_pud = pud_offset(heap_end_p4d, latest_task_info.heap_end_vaddr);
    pmd_t *heap_end_pmd = pmd_offset(heap_end_pud, latest_task_info.heap_end_vaddr);
    pte_t *heap_end_pte = pte_offset_kernel(heap_end_pmd, latest_task_info.heap_end_vaddr);
    latest_task_info.heap_end_paddr = (pte_val(*heap_end_pte) & PAGE_MASK) + (latest_task_info.heap_end_vaddr & ~PAGE_MASK);
    // Stack Area의 가상주소, 물리주소
    latest_task_info.stack_start_vaddr = latest_task->mm->start_stack;
    pgd_t *stack_start_pgd = pgd_offset(latest_task->mm, latest_task_info.stack_start_vaddr);
    p4d_t *stack_start_p4d = p4d_offset(stack_start_pgd, latest_task_info.stack_start_vaddr);
    pud_t *stack_start_pud = pud_offset(stack_start_p4d, latest_task_info.stack_start_vaddr);
    pmd_t *stack_start_pmd = pmd_offset(stack_start_pud, latest_task_info.stack_start_vaddr);
    pte_t *stack_start_pte = pte_offset_kernel(stack_start_pmd, latest_task_info.stack_start_vaddr);
    latest_task_info.stack_start_paddr = (pte_val(*stack_start_pte) & PAGE_MASK) + (latest_task_info.stack_start_vaddr & ~PAGE_MASK);
    latest_task_info.stack_end_vaddr = (unsigned long)latest_task->thread.sp; // thread의 sp(stack pointer)를 이용해 stack의 끝 주소를 구한다.
    pgd_t *stack_end_pgd = pgd_offset(latest_task->mm, latest_task_info.stack_end_vaddr);
    p4d_t *stack_end_p4d = p4d_offset(stack_end_pgd, latest_task_info.stack_end_vaddr);
    pud_t *stack_end_pud = pud_offset(stack_end_p4d, latest_task_info.stack_end_vaddr);
    pmd_t *stack_end_pmd = pmd_offset(stack_end_pud, latest_task_info.stack_end_vaddr);
    pte_t *stack_end_pte = pte_offset_kernel(stack_end_pmd, latest_task_info.stack_end_vaddr);
    latest_task_info.stack_end_paddr = (pte_val(*stack_end_pte) & PAGE_MASK) + (latest_task_info.stack_end_vaddr & ~PAGE_MASK);

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
            seq_printf(s, "PGD base address: 0x%lx\n", task_info_list[i].pgd); //시작 주소(포인터값) 출력
            // seq_printf(s, "Flags: 0x%lx\n", task_info_list[i].flags);
            // Code Area
            seq_printf(s, "Code Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", task_info_list[i].code_start_vaddr); // unsigned long, 16진수로 출력
            seq_printf(s, "- start (PGD): 0x%lx, 0x%lx\n", task_info_list[i].code_start_pgd, task_info_list[i].start_pgd_val); // pgd_t, pgd_val(*pgd)로 pgd가 가리키는 주소의 값(pud의 주소)을 가져온다.
            seq_printf(s, "- start (PUD): 0x%lx, 0x%lx\n", task_info_list[i].code_start_pud, task_info_list[i].start_pud_val);
            seq_printf(s, "- start (PMD): 0x%lx, 0x%lx\n", task_info_list[i].code_start_pmd, task_info_list[i].start_pmd_val);
            seq_printf(s, "- start (PTE): 0x%lx, 0x%lx\n", task_info_list[i].code_start_pte, task_info_list[i].start_pte_val);
            seq_printf(s, "- start (physical): 0x%lx\n", task_info_list[i].code_start_paddr);
            // seq_printf(s, "- start (physical1): 0x%lx\n", task_info_list[i].code_start_paddr1);
            seq_printf(s, "- end (virtual): 0x%lx\n", task_info_list[i].code_end_vaddr);
            seq_printf(s, "- end (PGD): 0x%lx, 0x%lx\n", task_info_list[i].code_end_pgd, task_info_list[i].end_pgd_val);
            seq_printf(s, "- end (PUD): 0x%lx, 0x%lx\n", task_info_list[i].code_end_pud, task_info_list[i].end_pud_val);
            seq_printf(s, "- end (PMD): 0x%lx, 0x%lx\n", task_info_list[i].code_end_pmd, task_info_list[i].end_pmd_val);
            seq_printf(s, "- end (PTE): 0x%lx, 0x%lx\n", task_info_list[i].code_end_pte, task_info_list[i].end_pte_val);
            seq_printf(s, "- end (physical): 0x%lx\n", task_info_list[i].code_end_paddr);
            // Data Area
            seq_printf(s, "Data Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", task_info_list[i].data_start_vaddr);
            seq_printf(s, "- start (physical): 0x%lx\n", task_info_list[i].data_start_paddr);
            seq_printf(s, "- end (virtual): 0x%lx\n", task_info_list[i].data_end_vaddr);
            seq_printf(s, "- end (physical): 0x%lx\n", task_info_list[i].data_end_paddr);
            // Heap Area
            seq_printf(s, "Heap Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", task_info_list[i].heap_start_vaddr);
            seq_printf(s, "- start (physical): 0x%lx\n", task_info_list[i].heap_start_paddr);
            seq_printf(s, "- end (virtual): 0x%lx\n", task_info_list[i].heap_end_vaddr);
            seq_printf(s, "- end (physical): 0x%lx\n", task_info_list[i].heap_end_paddr);
            // Stack Area
            seq_printf(s, "Stack Area\n");
            seq_printf(s, "- start (virtual): 0x%lx\n", task_info_list[i].stack_start_vaddr);
            seq_printf(s, "- start (physical): 0x%lx\n", task_info_list[i].stack_start_paddr);
            seq_printf(s, "- end (virtual): 0x%lx\n", task_info_list[i].stack_end_vaddr);
            seq_printf(s, "- end (physical): 0x%lx\n", task_info_list[i].stack_end_paddr);
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

    // my timer 초기화(시간이 되면 callback_timer 통해 tasklet_schedule 함수 호출되게 my timer 설정)
    timer_setup(&my_timer, callback_timer, 0);
    // 모듈 삽입 0.1초 후에 callback_timer 함수 호출해 tasklet_schedule 함수 호출
    // 그 후 10초마다 callback_timer 함수 안에서 다시 tasklet_schedule 함수 호출
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(100)); 

    return 0; 
}

static void __exit hello_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    del_timer_sync(&my_timer);
    tasklet_kill(&my_tasklet);
}

module_init(hello_init);
module_exit(hello_exit);
