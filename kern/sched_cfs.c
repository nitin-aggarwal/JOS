#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched_cfs.h>

//Global clock declaration
struct global_clock global_clock;


// Instantiates global clock values at the start of the program
void 
global_clock_init(void)
{
	global_clock.global_tick = 1;
	global_clock.real_tick = 0;
	//global_clock.max_real_tick = 0;
}

void
global_clock_increment(void)
{
	global_clock.global_tick++;
}


void 
sched_fair_yield(void)
{
	//Increment real_tick_used of global clock
        global_clock.real_tick++;
	
	int i, env_index = 1024;
	struct Env *e = NULL;
	
	//Find index in envs of curenv
	for(i = 0; i < NENV; i++)
	{
		e = &envs[i];
		if(e == curenv)
		{
			env_index = i;
			break;
		}
	}
	
	//Decrementing timeslice for current process as it has run once
	if(env_index > -1 && env_index < 1024)		// for scenarios when there is no current environment
		envs[env_index].env_se.timeslice--;

//	cprintf("Entering sched_fair_yield envID : %d , timeslice_remaining %d \n", env_index, envs[env_index].env_se.timeslice);

	i = env_index;
	
	if(envs[i].env_se.timeslice == 0)	
	{
	
		//Current process has finished one timeslice, so increment vruntime and reset timeslice	
	  	 if(env_index > -1 && env_index < 1024)
	   	{
			envs[i].env_se.vruntime++;
		
			// Reset the timeslice on the basis of priority
			envs[i].env_se.timeslice = MIN_TIMESLICE * (envs[i].env_se.priority - MIN_PRIORITY + 1);		
	   	}
	}	//Terminated this loop here
	   
	
	int flag = 0, k = 1;
	
	// variables to find the process to be scheduled next on the basis of  vruntime and timeslices left
	uint64_t min_vruntime = 0;
	int timesleft = 0;
		
		
	env_index = -1;			//setting env_index to -1, remains -1 if there is no env to be scheduled
	i = 1;				//CHECK THIS LINE LATER
	
	while(i > 0 && i < NENV)
	{
		if(i == NENV -1)
			i = 1;
		if(envs[i].env_status == ENV_RUNNABLE)// && envs[i].env_se.timeslice > 0)
		{
			//cprintf("i: %d\n",i);	
			if(flag == 0)
			{
				min_vruntime = envs[i].env_se.vruntime;
				env_index = i;
				timesleft = envs[i].env_se.timeslice;
				flag = 1;
			}
			else if(envs[i].env_se.vruntime < min_vruntime)
			{
				env_index = i;
				min_vruntime = envs[i].env_se.timeslice;
				timesleft = envs[i].env_se.timeslice;
			} 
			else if(envs[i].env_se.vruntime == min_vruntime && envs[i].env_se.timeslice > timesleft)
			{
				env_index = i;
				timesleft = envs[i].env_se.timeslice;
			}
		}
		k++;
		if(k == NENV)
			break;
		i++;
	}
	
	

	//Check if global clock has to be incremented
	if(global_clock.global_tick == min_vruntime)  	// Change comparison operaor from <= to ==
	{
		global_clock_increment();
	}

	cprintf("******* CF Scheduler Selected Process ENV-ID  [%08x] VRUNTIME: %llu  TIMESLICE-LEFT: %llu ********\n",envs[env_index].env_id,envs[env_index].env_se.vruntime,envs[env_index].env_se.timeslice);	
	if(env_index > 0)
		env_run(&envs[env_index]);
	else if(envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else
	{
		cprintf("Destroyed all environments - nothing more to do");
		while(1)
			monitor(NULL);
	}
	
}
