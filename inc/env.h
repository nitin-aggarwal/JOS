/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_ENV_H
#define JOS_INC_ENV_H

#include <inc/types.h>
#include <inc/queue.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t envid_t;

// An environment ID 'envid_t' has three parts:
//
// +1+---------------21-----------------+--------10--------+
// |0|          Uniqueifier             |   Environment    |
// | |                                  |      Index       |
// +------------------------------------+------------------+
//                                       \--- ENVX(eid) --/
//
// The environment index ENVX(eid) equals the environment's offset in the
// 'envs[]' array.  The uniqueifier distinguishes environments that were
// created at different times, but share the same environment index.
//
// All real environments are greater than 0 (so the sign bit is zero).
// envid_ts less than 0 signify errors.  The envid_t == 0 is special, and
// stands for the current environment.

#define LOG2NENV		10
#define NENV			(1 << LOG2NENV)
#define ENVX(envid)		((envid) & (NENV - 1))

// Values of env_status in struct Env
#define ENV_FREE		0
#define ENV_RUNNABLE		1
#define ENV_NOT_RUNNABLE	2

//Project
// Prioriies 
#define MAX_PRIORITY 		19
#define MIN_PRIORITY 		-20
#define MIN_TIMESLICE 		1 //minimum granularity per env run

//Lottery
#define MAX_ENV_TICKETS		10	//max tickets per env0
#define MAX_TOTAL_TICKETS	NENV * MAX_ENV_TICKETS

struct sched_entity
{
        //struct rb_node rb_node;
        int32_t priority;		//Priority of env -- Nice value
        uint64_t timeslice;		//Amount of time the env runs when it is scheduled (within one global tick)
	
	// Completely Fair Scheduler
	uint64_t vruntime;		//time process has run in terms of global ticks
        uint64_t start_time;    	//will contain value of global_tick when env created

	// O(1) Scheduler
	int32_t stat_priority;		// Static priority of the process
	int32_t dyna_priority;		// Dynamic priority of the process
	uint64_t sleep_avg;		// Average sleep time of the process
	uint64_t timestamp;		// Timestamp in term of timer interrupts of last process run
	
	//Lottery Scheduler
	uint32_t tickets[MAX_ENV_TICKETS];	// Array to store lottery tickets
	uint32_t ticket_count;			// Number of tickets allocated
};

struct Env {
	struct Trapframe env_tf;	// Saved registers
	LIST_ENTRY(Env) env_link;	// Free list link pointers
	envid_t env_id;			// Unique environment identifier
	envid_t env_parent_id;		// env_id of this env's parent
	unsigned env_status;		// Status of the environment
	uint32_t env_runs;		// Number of times environment has run

	// Address space
	pde_t *env_pgdir;		// Kernel virtual address of page dir
	physaddr_t env_cr3;		// Physical address of page dir

	// Exception handling
	void *env_pgfault_upcall;	// page fault upcall entry point

	// Lab 4 IPC
	bool env_ipc_recving;		// env is blocked receiving
	void *env_ipc_dstva;		// va at which to map received page
	uint32_t env_ipc_value;		// data value sent to us 
	envid_t env_ipc_from;		// envid of the sender	
	int env_ipc_perm;		// perm of page mapping received

	//Project
	struct sched_entity env_se;	//struct containing scheduling details of env
	LIST_ENTRY(Env) env_rqlink;   	// Runqueue link pointers
};
#endif // !JOS_INC_ENV_H
