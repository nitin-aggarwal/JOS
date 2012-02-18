/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_SCHED_H
#define JOS_KERN_SCHED_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/types.h>

//SCHED_CLASS selects scheduling algorithm to be used 
// 1 - RoundRobin, 2 - CFS, 3 - O(1), 4 - Lottery
#define SCHED_CLASS 1

// This function does not return.
void sched_rr_yield(void);
void sched_yield(void);// __attribute__((noreturn));

#endif	// !JOS_KERN_SCHED_H
