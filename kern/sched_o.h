#ifndef JOS_KERN_SCHED_O_H
#define JOS_KERN_SCHED_O_H

#include <inc/types.h>
#include <kern/sched_o.h>
#include <inc/env.h>
#include <kern/env.h>

void sched_init();
void update_sched_entity(struct Env *);
void task_timeslice(struct Env *);

int task_interactive(struct Env *);
void enqueue_env(struct Env*, struct prio_array_t*);
void dequeue_env(struct Env*, struct prio_array_t*);
void sched_o_yield(void) __attribute__((noreturn));
void clear_bit(uint32_t, uint8_t *);
void set_bit(uint32_t, uint8_t *);
int get_max_bit();
uint32_t env2envid(struct Env *);
void print_bitmap();
void swap_lists(uint32_t);	//Checks if active and expired runqueues need to be swapped
void enqueueX_env(struct Env*, struct prio_array_t*);
int is_active_bitmap_empty();
int is_expired_bitmap_empty();
void swap_full_lists();
#endif
