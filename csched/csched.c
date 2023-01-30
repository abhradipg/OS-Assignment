
#define _GNU_SOURCE
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/kthread.h>
#include <linux/sched/hotplug.h>
#include <linux/cpu.h>
#include <linux/completion.h>
#include <linux/cpuset.h>
#include <linux/stop_machine.h>
#include <../kernel/sched/sched.h>

struct task_struct *task;
int cpu;
cpumask_t *mask,*new_mask = NULL,temp;
int dest_cpu;

asmlinkage long __x64_sys_cpu_isolator(void) {
    
    //sched_cpu_dying(1);
    //clear_tasks_mm_cpumask(1);
    //cpumask_clear(&temp);
    //cpumask_set_cpu(1,&temp);
    //sched_setaffinity(task_pid_nr(current), &temp);
    for_each_process(task){

        new_mask = kmalloc(cpumask_size(), GFP_KERNEL);
		if (!new_mask){
            new_mask=&temp;
        }
			    
        cpumask_setall(new_mask);
        cpumask_clear_cpu(1,new_mask);

        if(!kthread_is_per_cpu(task)){
            
           // printk("old_mask-%*pbl", cpumask_pr_args(&task->cpus_mask));
            if(!cpumask_and(new_mask, &task->cpus_mask, new_mask)){
                cpumask_clear(new_mask);
                cpumask_set_cpu(0,new_mask);
            }
                task->migration_disabled=0;
                //wake_up_process(task);
                //sched_move_task(task);
                //set_tsk_need_resched(task);
                set_cpus_allowed_ptr(task,new_mask);
                
                //wake_up_new_task(task);
                dest_cpu = cpumask_any_and_distribute(cpu_active_mask, new_mask);
                if(task_cpu(task)==1){
                    //sched_move_task(task);
                    //set_task_cpu(task, dest_cpu);
                    //set_cpus_allowed_ptr(task,new_mask);
                    __migrate_swap_task(task, dest_cpu);
                   if(task_cpu(task)==1){
                       //printk("cant mig pid-[%d]", task->pid);
                       insert_migreq(task,dest_cpu);
                       set_tsk_need_resched(task);
                       wake_up_process(task);
                       set_cpus_allowed_ptr(task,new_mask);
                       migrate_task_to(task,dest_cpu);
                   }
                   //printk("pid-[%d]", task->pid);
                }
                //printk("dest-%d\n",dest_cpu);
                //migrate_task_to(task,dest_cpu);
                //printk("%d\n",ret);
           // printk("new mask - %*pbl", cpumask_pr_args(&task->cpus_mask));
        }
        else{
           // printk("Is per cpu kthread");
            //printk("pid-[%d]", task->pid);
            /*printk("old_mask-%*pbl", cpumask_pr_args(&task->cpus_mask));
            if(!cpumask_and(new_mask, &task->cpus_mask, new_mask)){
               kthread_bind(task,0);
            }
                //set_cpus_allowed_ptr(task,new_mask);
            printk("new mask - %*pbl", cpumask_pr_args(&task->cpus_mask));*/
            ;
        }
        if(new_mask!=&temp)
           kfree(new_mask);
    }
    //sched_cpu_deactivate(1);
    //sched_cpu_dying(1);
    //sched_cpu_starting(1);
    //sched_cpu_activate(1);

    return 0;
}