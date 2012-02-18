#ifndef JOS_KERN_PROJECT_SCHED_H
#define JOS_KERN_PROJECT_SCHED_H

#include <inc/types.h>

//Project
/*
struct rb_node
{
        unsigned long rb_parent_color;
#define RB_RED 0
#define RB_BLACK 1
        struct rb_node *rb_right;
        struct rb_node *rb_left;
};

struct rb_root
{
	struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root) { NULL, }


#define MAX_PRIORITY 1
#define MIN_PRIORITY 5
#define MIN_VRUNTIME 1	// minimum granularity per env run

struct sched_entity
{
        //struct rb_node rb_node;
        uint64_t priority;
	uint64_t min_vruntime;
	uint64_t vruntime;
	uint64_t start_time;	//will contain value of global_tick when env created
};
*/

#endif // JOS_KERN_PROJECT_SCHED_H
