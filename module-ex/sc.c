#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>   // task_struct 사용하기 위한 헤더 파일
                                   // find / -name "sched.h"  로 헤더 파일 위치를 찾는다
int __init init_test(void){   // 모듈이 장착될 때(insmod)
        struct task_struct *p;
        p = get_current();   // 현재 프로세스를 얻는다

        // 자식과 부모의 pid, state, comm 을 얻는다
        printk("[Kernel Message] : Currnet Process\n");
        printk("pid = %d, comm = %s\n", p->pid, p->comm); 
        struct task_struct *pa;
        pa = p->real_parent;   // 현재 프로세스가 가르키는 부모 프로세스를 얻는다

        printk("[Kernel Message] : Parent Process\n");
        printk("pid = %d, comm = %s\n", pa->pid, pa->comm);
        return 0;
}
void __exit exit_test(void){}   // 모듈이 해제될 때(rmmod)

module_init(init_test);
module_exit(exit_test);
MODULE_LICENSE("GPL");
