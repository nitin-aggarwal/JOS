#ifndef JOS_KERN_SCHED_LOTTERY_H
#define JOS_KERN_SCHED_LOTTERY_H

#include <kern/sched_lottery.h>
#include <inc/types.h>

extern uint32_t ticket_list[];
extern uint32_t ticket_count_issued;

void ticket_list_init(void);
int rand(uint32_t);

void ticket_alloc(struct Env*);
void ticket_free(struct Env*);

void sched_lottery_yield(void);

#endif 	// !JOS_KERN_SCHED_LOTTERY_H
