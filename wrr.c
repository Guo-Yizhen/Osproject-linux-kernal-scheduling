/*
 * WRR:Weighted Round Robin Scheduling (mapped to SCHED_WRR policy)
 * Allocated times slice depends on task's taskgroup
 * foreground or background
 */

#include "sched.h"
#include <linux/slab.h>

#define PATH_LENGTH 1000
#define BACKGROUND "/bg_non_interactive"
#define FOREGROUND "/"
/*
 *   PART: pre-work
 *	 all the simple function needed
 */
static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se ,struct task_struct, wrr);
}

static inline struct rq *rq_of_wrr_rq(struct wrr_rq *wrr_rq)
{
	return container_of(wrr_rq, struct rq, wrr);
}

static inline struct wrr_rq *wrr_rq_of_se(struct sched_wrr_entity *wrr_se)
{
    struct task_struct *p = wrr_task_of(wrr_se);
    struct rq *rq = task_rq(p);
    return &rq->wrr;
}

static inline int on_wrr_rq(struct sched_wrr_entity *wrr_se)
{
	return !list_empty(&wrr_se->run_list);
}
static inline
void inc_wrr_tasks(struct sched_wrr_entity *wrr_se, struct wrr_rq *wrr_rq)
{
	wrr_rq->wrr_nr_running++;
}
static inline
void dec_wrr_tasks(struct sched_wrr_entity *wrr_se, struct wrr_rq *wrr_rq)
{
	WARN_ON(!wrr_rq->wrr_nr_running);
	wrr_rq->wrr_nr_running--;
}

/*
 *   PART: enqueue
 *	 enqueue the entity to the head or tail of runqueue it belongs to
 */

static void enqueue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se, bool head)
{
	printk("	@ enqueue_wrr_entity:  working\n");
    struct list_head *queue = &rq->wrr.queue;
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
    if (head)
        list_add(&wrr_se->run_list, queue);
    else
        list_add_tail(&wrr_se->run_list, queue);

    inc_wrr_tasks(wrr_se,wrr_rq);
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags){
    printk("	@ enqueue_task_wrr:  working\n");
    struct sched_wrr_entity *wrr_se = &p->wrr;

	if (flags & ENQUEUE_WAKEUP)
		wrr_se->timeout = 0;

	enqueue_wrr_entity( rq, wrr_se, flags & ENQUEUE_HEAD);

	inc_nr_running(rq);
}

/*
 *   PART: dequeue
 *	 first update the current wrr runqueue
 *	 then delete from list
 */

static void update_curr_wrr(struct rq *rq)
{
	printk("		@ Updating_curr_wrr:  working\n");
	struct task_struct *curr = rq->curr;
	u64 delta_exec;

	if (curr->sched_class != &wrr_sched_class)
		return;

	delta_exec = rq->clock_task - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max,
		      max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

static void dequeue_wrr_entity(struct rq *rq, struct sched_wrr_entity *wrr_se)
{
	printk("	@ dequeue_wrr_entity:  working\n");
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
    list_del_init(&wrr_se->run_list);
    dec_wrr_tasks(wrr_se, wrr_rq);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	printk("	@ dequeue_task_wrr:  working\n");
    struct sched_wrr_entity *wrr_se = &p->wrr;

	update_curr_wrr(rq);
	dequeue_wrr_entity(rq,wrr_se);

	dec_nr_running(rq);
}

/*
 *   PART: yeild_task and requeue
 *   Put task to the end of the run list
 */

static void requeue_task_wrr(struct rq *rq, struct task_struct *p, int head)
{
	printk("	@ requeue_task_wrr:  working\n");
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq = wrr_rq_of_se(wrr_se);
	if (on_wrr_rq(wrr_se)) {
		struct list_head *queue = &wrr_rq->queue; 
		list_move_tail(&wrr_se->run_list, queue);
	}
}

static void yield_task_wrr(struct rq *rq)
{
    printk("	@ yield_task_wrr:  working\n");
	requeue_task_wrr(rq, rq->curr, 0);

}

/*
 *   PART: pick and put
 *	 return the next task on the runqueue
 */
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
    //printk("	@ pick_next_task_wrr: working\n");
    struct sched_wrr_entity* wrr_se;
    struct task_struct *p;

    if (rq->wrr.wrr_nr_running == 0)
        return NULL;
    wrr_se = list_first_entry(&rq->wrr.queue, struct sched_wrr_entity, run_list); 
    p = wrr_task_of(wrr_se);  
    if (!p)  return NULL;
    p->se.exec_start = rq->clock_task;  
	return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	printk("	@ put_prev_task_wrr: working\n");
	update_curr_wrr(rq);
	p->se.exec_start = 0;
}

/*
 *     PART: task fork
 *     refer to task_fork_fair(struct task_struct *p)
 */
static void task_fork_wrr(struct task_struct *p)
{
    printk("	@ Wtask_fork_wrr:  working\n");
	struct rq *rq = this_rq();
	int this_cpu = smp_processor_id();
    unsigned long flags;

    raw_spin_lock_irqsave(&rq->lock, flags);

    update_rq_clock(rq);
    p->wrr.time_slice = p->wrr.parent->time_slice;
    rcu_read_lock();
    __set_task_cpu(p, this_cpu);
    rcu_read_unlock();

    raw_spin_unlock_irqrestore(&rq->lock, flags);
}

/*
 *     PART: get interval
 *     return the timeslice of wrr
 */
static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	printk("		@ get_rr_interval_wrr:  working\n");

    char path_group[PATH_LENGTH];
	cgroup_path(task->sched_task_group->css.cgroup, path_group, PATH_LENGTH);
    if (task->policy == SCHED_WRR){
        if(!strcmp(path_group, FOREGROUND)){
			printk("			in foreground\n");
            return WRR_F_TIMESLICE;
		}
        if(!strcmp(path_group, BACKGROUND)){ 
			printk("			in background\n");
            return WRR_B_TIMESLICE;
		}
    }
	return 0;
}

/*
 *     PART: task tick
 *     dec the timeslice of wrr and dicide whether it needs resche
 */

static void watchdog(struct rq *rq, struct task_struct *p)
{
	unsigned long soft, hard;

	soft = task_rlimit(p, RLIMIT_RTTIME);
	hard = task_rlimit_max(p, RLIMIT_RTTIME);

	if (soft != RLIM_INFINITY) {
		unsigned long next;

		p->wrr.timeout++;
		next = DIV_ROUND_UP(min(soft, hard), USEC_PER_SEC/HZ);
		if (p->wrr.timeout > next)
			p->cputime_expires.sched_exp = p->se.sum_exec_runtime;
	}
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	printk("	@ task_tick_wrr:  working\n");
	struct sched_wrr_entity *wrr_se = &p->wrr;
	update_curr_wrr(rq);
	watchdog(rq, p);
	/*
	 * Only wrr policy
	 */
	if (--p->wrr.time_slice)
		return;
    p->wrr.time_slice = get_rr_interval_wrr(NULL, p);   

    if(p->wrr.time_slice == 0){
        printk("@ error: task_tick_wrr p got wrong time_slice\n");
        return;
    }
	
	struct list_head *queue = &rq->wrr.queue;
	if (queue->prev != queue->next) {
		requeue_task_wrr(rq, p, 0);
		resched_task(p);
	}
}

/*
 *     PART: set curr task
 */

static void set_curr_task_wrr(struct rq *rq)
{
	printk("	@ set_curr_task_wrr:  working\n");
    struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock_task;
}

/*
 *     PART: switch to wrr
 *     resche the curr task  if it is no WRR
 */
static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
    printk("	@ switched_to_wrr: working\n");
    if (p->on_rq && rq->curr != p)
		if (rq == task_rq(p) && rq->curr->policy != SCHED_WRR)
			resched_task(rq->curr);
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{ return 0; }
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{}
static void set_cpus_allowed_wrr(struct task_struct *p, const struct cpumask *new_mask)
{}
static void rq_online_wrr(struct rq *rq)
{}
static void rq_offline_wrr(struct rq *rq)
{}
static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev)
{}
static void post_schedule_wrr(struct rq *rq)
{}
static void task_woken_wrr(struct rq *rq, struct task_struct *p)
{}
static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{}

static void switched_from_wrr(struct rq *rq, struct task_struct *p)
{}
void free_wrr_sched_group(struct task_group *tg)
{}
extern int alloc_wrr_sched_group(struct task_group *tg, struct task_group *parent)
{ return 1; }

const struct sched_class wrr_sched_class = {
    .next = &fair_sched_class,                      /*Required*/
    .enqueue_task = enqueue_task_wrr,               /*Required*/
    .dequeue_task = dequeue_task_wrr,               /*Required*/
    .yield_task = yield_task_wrr,                   /*Required*/
    .check_preempt_curr = check_preempt_curr_wrr,   /*Required*/
    .pick_next_task = pick_next_task_wrr,           /*Required*/
    .put_prev_task = put_prev_task_wrr,             /*Required*/
    .task_fork = task_fork_wrr,                     /*Required*/
#ifdef CONFIG_SMP
    .select_task_rq = select_task_rq_wrr,           /*Never need impl*/
    .set_cpus_allowed = set_cpus_allowed_wrr,       /*Never need impl*/
    .rq_online = rq_online_wrr,                     /*Never need impl*/
    .rq_offline = rq_offline_wrr,                   /*Never need impl*/
    .pre_schedule = pre_schedule_wrr,               /*Never need impl*/
    .post_schedule = post_schedule_wrr,             /*Never need impl*/
    .task_woken = task_woken_wrr,                   /*Never need impl*/
#endif
    .set_curr_task = set_curr_task_wrr,             /*Required*/
    .task_tick = task_tick_wrr,                     /*Required*/
    .get_rr_interval = get_rr_interval_wrr,         /*Required*/

    .prio_changed = prio_changed_wrr,               /*Never need impl*/
    .switched_to = switched_to_wrr,                 /*actually Required*/
    .switched_from = switched_from_wrr,             /*Never need impl*/
};
