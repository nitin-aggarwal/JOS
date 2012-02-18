#ifndef JOS_KERN_SCHED_CFS_H
#define JOS_KERN_SCHED_CFS_H

#include <kern/sched_cfs.h>
#include <inc/types.h>

extern struct global_clock global_clock;

struct global_clock
{
	uint64_t global_tick;		//The global clock
	uint64_t real_tick;		//Real clock	
};

//extern struct global_clock global_clock;

void global_clock_init(void);
void global_clock_increment(void);
//int has_timeslice(envid_t envid);

//function does not return
void sched_fair_yield(void) __attribute__((noreturn));

#endif // !JOS_KERN_SCHED_CFS_H
