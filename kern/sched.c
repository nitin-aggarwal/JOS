#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched_cfs.h>
#include <kern/sched_o.h>
#include <kern/sched.h>
#include <kern/sched_lottery.h>

//SCHED_CLASS selects scheduling algorithm to be used 
// 1 - RoundRobin, 2 - CFS, 3 - O(1), 4- Lottery

// Choose a user environment to run and run it.
void
sched_yield(void)
{
//	cprintf("*************Process Scheduled**********\n");
//PLEASE DONT CHANGE ORDER 1 - RoundRobin, 2 - CFS, 3 - O(1), 4 - Lottery
	switch(SCHED_CLASS)
	{
		case 1: sched_rr_yield();
			break;
		case 2: sched_fair_yield();
			break;
		case 3: sched_o_yield();
			break;
		case 4: sched_lottery_yield();
			break;
		default:
			break;
	}
}

void
sched_rr_yield(void)
{
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.

	// LAB 4: Your code here.
	int i,envIndex = 0;
	struct Env *e = NULL;
	for(i=0;i< NENV;i++)
	{
		e = &envs[i];
		if(e == curenv)
		{
			envIndex = i;
			break;
		}
	}
	i = envIndex + 1;
	int k = 1;
	while(i > 0 && i < NENV)
	{
		if(i == NENV -1)
			i = 1;
		if(envs[i].env_status == ENV_RUNNABLE)
		{
			env_run(&envs[i]);
			return;
		}
		k++;
		if(k == NENV)
			break;
		i++;
	}	
	// Run the special idle environment when nothing else is runnable.
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
