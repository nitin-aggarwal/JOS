/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_ENV_H
#define JOS_KERN_ENV_H

#include <inc/env.h>

#ifndef JOS_MULTIENV
// Change this value to 1 once you're allowing multiple environments
// (for UCLA: Lab 3, Part 3; for MIT: Lab 4).
#define JOS_MULTIENV 0
#endif

extern struct Env *envs;		// All environments
extern struct Env *curenv;		// Current environment

LIST_HEAD(Env_list, Env);		// Declares 'struct Env_list'

void	env_init(void);
int	env_alloc(struct Env **e, envid_t parent_id);
void	env_free(struct Env *e);
void	env_create(uint8_t *binary, size_t size);
void	env_destroy(struct Env *e);	// Does not return if e == curenv

int	envid2env(envid_t envid, struct Env **env_store, bool checkperm);
// The following two functions do not return
void	env_run(struct Env *e) __attribute__((noreturn));
void	env_pop_tf(struct Trapframe *tf) __attribute__((noreturn));

//Project
extern struct runqueue rq;

void	env_create_scheduler(uint8_t *binary, size_t size, int32_t value);

struct prio_array_t {

        uint8_t bitmap[5];       // Bitmap for priority lists elemnts presence
        struct Env_list pqueue[40];   // List for each priority

};


struct runqueue {

        struct prio_array_t* active;    // Pointer to the list of active processes
        struct prio_array_t* expired;   // Pointer to the list of expired processes

        struct prio_array_t arrays[2];  // Sets of active and expired processes

};

// For the grading script
#define ENV_CREATE2(start, size)	{		\
	extern uint8_t start[], size[];			\
	env_create(start, (int)size);			\
}

#define ENV_CREATE(x)			{		\
	extern uint8_t _binary_obj_##x##_start[],	\
		_binary_obj_##x##_size[];		\
	env_create(_binary_obj_##x##_start,		\
		(int)_binary_obj_##x##_size);		\
}

#define ENV_CREATE_SCHEDULER(x,value)			{		\
	extern uint8_t _binary_obj_##x##_start[],	\
		_binary_obj_##x##_size[];		\
	env_create_scheduler(_binary_obj_##x##_start,		\
		(int)_binary_obj_##x##_size,		\
		(int32_t)value);			\
}

#endif // !JOS_KERN_ENV_H
