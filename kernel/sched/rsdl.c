#include <linux/energy_model.h>
#include <linux/mmap_lock.h>
#include <linux/hugetlb_inline.h>
#include <linux/jiffies.h>
#include <linux/mm_api.h>
#include <linux/highmem.h>
#include <linux/spinlock_api.h>
#include <linux/cpumask_api.h>
#include <linux/lockdep_api.h>
#include <linux/softirq.h>
#include <linux/refcount_api.h>
#include <linux/topology.h>
#include <linux/sched/clock.h>
#include <linux/sched/cond_resched.h>
#include <linux/sched/cputime.h>
#include <linux/sched/isolation.h>
#include <linux/sched/nohz.h>

#include <linux/cpuidle.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/mutex_api.h>
#include <linux/profile.h>
#include <linux/psi.h>
#include <linux/ratelimit.h>
#include <linux/task_work.h>

#include <asm/switch_to.h>

#include <linux/sched/cond_resched.h>

#include "sched.h"
#include "stats.h"
#include "autogroup.h"


const struct sched_class rsdl_sched_class;



static void update_curr_rsdl(struct rq *rq)
{
    
	u64 curr_time=rq->clock;
	struct task_struct *curr;
	struct rsdl_node *prev,*temp;
    u64 delta;
	//printk("Inside update_curr rsdl---------\n");
	if(rq->curr==NULL){
		return;
	}
	curr=rq->curr;
	delta=curr_time-curr->rsdl_start;
	curr->rsdl_start=curr_time;
	//printk("%llu\n",curr->remaining);
	//printk("%d\n",curr->currqueue);
	//printk("%d",curr->pid);
	if(delta>curr->remaining){
		prev=NULL;
		temp=NULL;
		temp=rq->rsdl.arrayoftask.active[curr->currqueue].start;
        while(temp!=NULL){
			if(temp->p->pid==rq->curr->pid){
				if(rq->rsdl.arrayoftask.active[curr->currqueue].start->next!=NULL){
				   if(prev==NULL)
				      rq->rsdl.arrayoftask.active[curr->currqueue].start=temp->next;
					else{
						prev->next=temp->next;
					}
				   temp->next=NULL;
				   rq->rsdl.arrayoftask.active[curr->currqueue].end->next=temp;
                   rq->rsdl.arrayoftask.active[curr->currqueue].end=temp;
			    }
			    break;	
			}

			prev=temp;
			temp=temp->next;
		}
		curr->remaining=5000000ULL;
		resched_curr(rq);
	}
	else{
		curr->remaining-=delta;
	}
}

static inline void
update_stats_wait_start_rsdl(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	//printk("Inside update stats wait start rsdl---------\n");
}

static inline void
update_stats_wait_end_rsdl(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
         //printk("Inside update stats wait end rsdl---------\n");
}

static inline void
update_stats_enqueue_sleeper_rsdl(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	//printk("Inside upadte stats wait enqueue sleeper rsdl---------\n");
}


static inline void
update_stats_enqueue_rsdl(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
	//printk("Inside upadte stats enque rsdl---------\n");
}

static inline void
update_stats_dequeue_rsdl(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{

    //printk("Inside upadte stats dequeue rsdl---------\n");
}


void set_task_rq_rsdl(struct sched_entity *se,
		      struct cfs_rq *prev, struct cfs_rq *next)
{
	//printk("Inside set task rq rsdl---------\n");
}




static void
enqueue_task_rsdl(struct rq *rq, struct task_struct *p, int flags)
{
	struct rsdl_node *newnode=kmalloc(sizeof(struct rsdl_node),GFP_KERNEL);
	int insertToPos;
	if(p->on_rsdl)
	   return;
	//printk("Inside enque rsdl---------\n");
	//printk("%d",p->pid);
	//printk("%d",rq->cpu);
    p->on_rsdl=1;
	p->rsdl_start=rq->clock;
	p->remaining=5000000ULL;
	p->quota=5000000ULL;
	newnode->p=p;
	newnode->next=NULL;
	insertToPos=task_prio(p);
	if(rq->rsdl.rsdl_nr_running==0){
		for(int i=0;i<41;i++){
			rq->rsdl.arrayoftask.active[i].start=NULL;
            rq->rsdl.arrayoftask.active[i].end=NULL;
            rq->rsdl.arrayoftask.active[i].nr_processes=0;
		}
	}
	for(;insertToPos<=40;insertToPos++){
		if(rq->rsdl.arrayoftask.active[insertToPos].remaining>0ULL)
		   break;
	}
	p->currqueue=insertToPos;
	//printk("%d",insertToPos);
    if(rq->rsdl.arrayoftask.active[insertToPos].nr_processes==0){
		//printk("inserted at %d",insertToPos);
        rq->rsdl.arrayoftask.active[insertToPos].start=newnode;
		rq->rsdl.arrayoftask.active[insertToPos].end=newnode;
	}
	else if(rq->rsdl.arrayoftask.active[insertToPos].nr_processes==1){
		rq->rsdl.arrayoftask.active[insertToPos].start->next=newnode;
        rq->rsdl.arrayoftask.active[insertToPos].end->next=newnode;
		rq->rsdl.arrayoftask.active[insertToPos].end=newnode;
	}
	else{
		rq->rsdl.arrayoftask.active[insertToPos].end->next=newnode;
		rq->rsdl.arrayoftask.active[insertToPos].end=newnode;
	}
	rq->rsdl.arrayoftask.active[insertToPos].nr_processes++;
	rq->rsdl.rsdl_nr_running++;
	p->on_rq=1;
	//printk("Outside enque rsdl---------\n");
}

static void rsdl_flip(struct rq *rq){
	struct rsdl_node *temp,*ptemp;
	//printk("Inside rsdl filp-----\n");
	for(int i=0;i<40;i++){
		rq->rsdl.arrayoftask.active[i].nr_processes=0;
		rq->rsdl.arrayoftask.active[i].alloted=20000000ULL;
		rq->rsdl.arrayoftask.active[i].start_time=rq->clock;
		rq->rsdl.arrayoftask.active[i].remaining=20000000ULL;
		rq->rsdl.arrayoftask.active[i].start=NULL;
		rq->rsdl.arrayoftask.active[i].end=NULL;
	}
    temp=rq->rsdl.arrayoftask.active[40].start;
	rq->rsdl.rsdl_nr_running=0;
	while(temp!=NULL){
		temp->p->on_rsdl=0;
		enqueue_task_rsdl(rq,temp->p,0);
		ptemp=temp;
		temp=temp->next;
		kfree(ptemp);
	}
	rq->rsdl.arrayoftask.active[40].nr_processes=0;
	rq->rsdl.arrayoftask.active[40].alloted=20000000ULL;
	rq->rsdl.arrayoftask.active[40].start_time=rq->clock;
	rq->rsdl.arrayoftask.active[40].remaining=20000000ULL;
	rq->rsdl.arrayoftask.active[40].start=NULL;
	rq->rsdl.arrayoftask.active[40].end=NULL;
	resched_curr(rq);
}

static void dequeue_task_rsdl(struct rq *rq, struct task_struct *p, int flags)
{
	struct rsdl_node *temp,*ptemp=NULL;
	if(!p->on_rsdl)
	   return;
    p->on_rsdl=0;
	printk("Inside deque rsdl---------\n");
	if(p->currqueue>40||p->currqueue<0)
	   return;
    temp=rq->rsdl.arrayoftask.active[p->currqueue].start;
	while(temp!=NULL){
		if(temp->p->pid==p->pid){
			printk("process found-------\n");
			if(temp==rq->rsdl.arrayoftask.active[p->currqueue].start)
			   rq->rsdl.arrayoftask.active[p->currqueue].start=temp->next;
			if(temp==rq->rsdl.arrayoftask.active[p->currqueue].end)
			   rq->rsdl.arrayoftask.active[p->currqueue].end=ptemp;
			if(ptemp!=NULL){
				ptemp->next=temp->next;
			}
			rq->rsdl.arrayoftask.active[p->currqueue].nr_processes--;
			rq->rsdl.rsdl_nr_running--;
			kfree(temp);
			break;
		}
        ptemp=temp;
		temp=temp->next;
	}
}


static int
select_task_rq_rsdl(struct task_struct *p, int prev_cpu, int wake_flags)
{
	//printk("Inside select task rq rsdl---------\n");
	return 0;
}


static void migrate_task_rq_rsdl(struct task_struct *p, int new_cpu)
{
     //printk("Inside migrate task rq rsdl---------\n");
}

static void task_dead_rsdl(struct task_struct *p)
{
	//struct rq *rq=this_rq();
	//resched_curr(this_rq());
	//for(int i=0;i<=40;i++){
    //   printk("-%d-",rq->rsdl.arrayoftask.active[i].nr_processes);
	//}
    
}

static int
balance_rsdl(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	//printk("Inside balance rsdl---------\n");
	return 0;
}



static struct task_struct *pick_task_rsdl(struct rq *rq)
{
	//printk("Inside pick task rsdl---------\n");
	return NULL;
}
//&&(!test_tsk_need_resched(temp->p))
struct task_struct *
pick_next_task_rsdl(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct rsdl_node *temp;
	int needflip=1;
	//printk("Inside next pick rsdl---------\n");
    if (!sched_rsdl_runnable(rq)){
		//printk("notask");
        return NULL;
	}
	for(int i=0;i<40;i++){
		if(rq->rsdl.arrayoftask.active[i].nr_processes>0)
		needflip=0;
	}
	if(needflip)
	  rsdl_flip(rq);
    for(int i=0;i<40;i++){
        if(rq->rsdl.arrayoftask.active[i].start!=NULL){
            temp=rq->rsdl.arrayoftask.active[i].start;
            while(temp!=NULL){
				if(temp->p!=NULL&&temp->p->__state==TASK_RUNNING){
					//printk("Inside next active pick rsdl---------\n%d",temp->p->currqueue);
					if(rq->rsdl.curr_queue!=i){
                         rq->rsdl.arrayoftask.active[i].start_time=rq->clock;
						 rq->rsdl.curr_queue=i;
					}
					temp->p->currqueue=i;
					if(temp->p==rq->curr)
					    return temp->p;
					temp->p->rsdl_start=rq->clock;
					rq->rsdl.curr=temp->p;
					return temp->p;
				}
				temp=temp->next;
			}
		}
	}
	//printk("Outside next pick rsdl---------\n");
	//rsdl_flip(rq);
	return NULL;

}

static struct task_struct *__pick_next_task_rsdl(struct rq *rq)
{
	return pick_next_task_rsdl(rq, NULL, NULL);
}

static void put_prev_task_rsdl(struct rq *rq, struct task_struct *prev)
{
	//printk("Inside put prev task rsdl---------\n");
	//rq->rsdl.curr_queue=prev->currqueue;
	//prev->rsdl_start=rq->clock;
	//rq->rsdl.arrayoftask.active[prev->currqueue].start_time=rq->clock;
}

static void yield_task_rsdl(struct rq *rq)
{
	//printk("Inside yield task rsdl---------\n");
}

static bool yield_to_task_rsdl(struct rq *rq, struct task_struct *p)
{
	//printk("Inside yeild to task rsdl---------\n");
	return false;
}

static void rq_online_rsdl(struct rq *rq)
{
	//printk("Inside rq online rsdl---------\n");
}

static void rq_offline_rsdl(struct rq *rq)
{
	//printk("Inside rq offline rsdl---------\n");
}


static void task_tick_rsdl(struct rq *rq, struct task_struct *curr, int queued)
{
	u64 curr_time=rq->clock;
	u64 delta;
	int curr_queue=rq->rsdl.curr_queue,needresched=0,needflip=1;
    struct rsdl_node *temp;
	//printk("Inside task tick rsdl----------\n");
	update_curr_rsdl(rq);
	delta=curr_time-rq->rsdl.arrayoftask.active[curr_queue].start_time;
	if(delta>rq->rsdl.arrayoftask.active[curr_queue].remaining){
		rq->rsdl.arrayoftask.active[curr_queue].remaining=0ULL;
		rq->rsdl.curr_queue++;
		needresched=1;
	}
	else{
		rq->rsdl.arrayoftask.active[curr_queue].remaining-=delta;
	}
	
	for(int i=0;i<40;i++){
		rq->rsdl.arrayoftask.active[i].start_time=curr_time;
		if(rq->rsdl.arrayoftask.active[i].remaining==0ULL&&rq->rsdl.arrayoftask.active[i].nr_processes!=0){
			temp=rq->rsdl.arrayoftask.active[i].start;
			while(temp!=NULL){
				temp->p->currqueue=i+1;
				temp=temp->next;
			}
			if(rq->rsdl.arrayoftask.active[i+1].nr_processes==0){
				rq->rsdl.arrayoftask.active[i+1].start=rq->rsdl.arrayoftask.active[i].start;
				rq->rsdl.arrayoftask.active[i+1].end=rq->rsdl.arrayoftask.active[i].end;
			}
			else{
				rq->rsdl.arrayoftask.active[i+1].end->next=temp;
				rq->rsdl.arrayoftask.active[i+1].end=rq->rsdl.arrayoftask.active[i].end;
			}
			rq->rsdl.arrayoftask.active[i+1].nr_processes+=rq->rsdl.arrayoftask.active[i].nr_processes;
			rq->rsdl.arrayoftask.active[i].nr_processes=0;
			
		}
	}
    for(int i=0;i<40;i++){
		if(rq->rsdl.arrayoftask.active[i].nr_processes>0)
		needflip=0;
	}
	rq->rsdl.curr_queue=curr->currqueue;
	if(needflip)
	   rsdl_flip(rq);
	if(needresched)
	   resched_curr(rq);
}


static void task_fork_rsdl(struct task_struct *p)
{
	printk("Inside task fork rsdl---------\n");
	//resched_curr(this_rq());
}


static void
prio_changed_rsdl(struct rq *rq, struct task_struct *p, int oldprio)
{
	//printk("Inside prio changed rsdl---------\n");
			//dequeue_task_rsdl(rq,p, 0);
		//enqueue_task_rsdl(rq,p, 0);
}

static void switched_from_rsdl(struct rq *rq, struct task_struct *p)
{
	//printk("Inside switched from rsdl---------\n");
}

static void switched_to_rsdl(struct rq *rq, struct task_struct *p)
{
	//printk("Inside switched to rsdl---------\n");
}


static void set_next_task_rsdl(struct rq *rq, struct task_struct *p, bool first)
{
	//printk("Inside set next task rsdl---------\n");
	rq->rsdl.curr_queue=p->currqueue;
	p->rsdl_start=rq->clock;
	rq->rsdl.arrayoftask.active[p->currqueue].start_time=rq->clock;
}


static unsigned int get_rr_interval_rsdl(struct rq *rq, struct task_struct *task)
{
    //printk("Inside get rr interval rsdl---------\n");
	return 1;
}
static void check_preempt_curr_rsdl(struct rq *rq, struct task_struct *p, int wake_flags)
{
	//printk("Inside check permt curr rsdl---------\n");
	if(task_prio(p) < rq->rsdl.curr_queue){
		//resched_curr(rq);
		return;
	}
}
/*
 * All the scheduling class methods:
 */
DEFINE_SCHED_CLASS(rsdl) = {

	.enqueue_task		= enqueue_task_rsdl,
	.dequeue_task		= dequeue_task_rsdl,
	.yield_task		= yield_task_rsdl,
	.yield_to_task		= yield_to_task_rsdl,

	.check_preempt_curr	= check_preempt_curr_rsdl,

	.pick_next_task		= __pick_next_task_rsdl,
	.put_prev_task		= put_prev_task_rsdl,
	.set_next_task          = set_next_task_rsdl,

#ifdef CONFIG_SMP
	.balance		= balance_rsdl,
	.pick_task		= pick_task_rsdl,
	.select_task_rq		= select_task_rq_rsdl,
	.migrate_task_rq	= migrate_task_rq_rsdl,

	.rq_online		= rq_online_rsdl,
	.rq_offline		= rq_offline_rsdl,

	.task_dead		= task_dead_rsdl,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.task_tick		= task_tick_rsdl,
	.task_fork		= task_fork_rsdl,

	.prio_changed		= prio_changed_rsdl,
	.switched_from		= switched_from_rsdl,
	.switched_to		= switched_to_rsdl,

	.get_rr_interval	= get_rr_interval_rsdl,

	.update_curr		= update_curr_rsdl,

#ifdef CONFIG_RSDL_GROUP_SCHED
	.task_change_group	= task_change_group_rsdl,
#endif

#ifdef CONFIG_UCLAMP_TASK
	.uclamp_enabled		= 1,
#endif
};


__init void init_sched_rsdl_class(void)
{

}

void init_rsdl_rq(struct rsdl_rq *rsdl_rq){
	for(int i=0;i<41;i++){
         rsdl_rq->arrayoftask.active[i].end=NULL;
		 rsdl_rq->arrayoftask.active[i].start=NULL;
		 rsdl_rq->arrayoftask.active[i].alloted=20000000ULL;
		 rsdl_rq->arrayoftask.active[i].remaining=20000000ULL;
		 rsdl_rq->arrayoftask.active[i].nr_processes=0;
	}
	rsdl_rq->rsdl_nr_running=0;
    rsdl_rq->curr_queue=0;
    raw_spin_lock_init(&rsdl_rq->rsdl_runtime_lock);

}
