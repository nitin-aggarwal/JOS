#include <inc/env.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched_cfs.h>
#include <kern/sched_o.h>

#define debug	0

void
sched_init()
{
	int i;
	rq.active = rq.arrays;
	rq.expired = rq.arrays + 1;

	// Clearing out all the lists in the beginning
	for(i=0;i<40;i++)
	{
		LIST_INIT(rq.active->pqueue + i);
		LIST_INIT(rq.expired->pqueue + i);
	}
}

void
update_sched_entity(struct Env *e)
{
	int64_t sleep;
	
	// Update the sleep_avg attribute
	sleep = global_clock.real_tick - e->env_se.timestamp -1;
	
	if(sleep < 0)
		sleep = 0;

	e->env_se.sleep_avg = (e->env_se.sleep_avg + sleep)/2;
	if(e->env_se.sleep_avg > 10000)
		e->env_se.sleep_avg = 10000;	// Sleep is upper bounded by 10^9  ticks  

	// Update the timestamp
	e->env_se.timestamp = global_clock.real_tick;

	// Update the dynamic priority
	int32_t bonus,d_priority,s_priority;

	s_priority = e->env_se.stat_priority;
	bonus = (int32_t)(e->env_se.sleep_avg/1000);
	d_priority = MAX(100,MIN(s_priority - bonus + 5,139));

	e->env_se.dyna_priority = d_priority;

	if(debug)
		cprintf("Avg_SLEEEP : %llu S Prio: %d D Prio: %d\n",e->env_se.sleep_avg,e->env_se.stat_priority,e->env_se.dyna_priority);
}

int
task_interactive(struct Env *e)
{
	int32_t bonus = (int32_t)(e->env_se.sleep_avg/1000);  	// Bonus computed on the basis of sleep_avg
	int32_t int_delta;
	
	int_delta = (e->env_se.stat_priority/4) - 28; 	// Represents the interactive delta
	int32_t temp = bonus - 5;
	
	if(debug)
	cprintf("Bonus-5 : %d interactive: %d\n ",temp ,int_delta);
	
	if(temp >= int_delta)
		return 1;
	else
		return 0;
	
}

void
task_timeslice(struct Env *e)
{
	int32_t s_priority,timeslice;

	s_priority = e->env_se.stat_priority;

	if(s_priority < 120)
		timeslice = (140 - s_priority) * 4;
	else
		timeslice = (140 - s_priority) * 1;
	
	// Update the timeslice of the process on the basis of static priority
	e->env_se.timeslice = timeslice;
}

void
enqueue_env(struct Env *e, struct prio_array_t *array)
{
	if(debug)
                cprintf("Entering enqueue in active list for env_id [%08x]\n", e->env_id);

	int32_t d_priority;
	
	update_sched_entity(e);

	d_priority = e->env_se.dyna_priority - 100;
	
	// If the list is empty, we have to set the bit to 1 for the priority queue
	if(LIST_EMPTY(array->pqueue + d_priority))
	{
		set_bit(d_priority,array->bitmap);
		LIST_INSERT_HEAD(array->pqueue + d_priority,e,env_rqlink);
	}
	else
	{
		struct Env *env;
		int i = 0,j = 0;
		
		// Finds the index of last element in the list
		LIST_FOREACH(env,array->pqueue+d_priority,env_rqlink){
			i++;
		}
	
		//Insert after the last elemnt in the list
		LIST_FOREACH(env,array->pqueue+d_priority,env_rqlink){
			j++;
			if(j == i)
				LIST_INSERT_AFTER(env,e,env_rqlink);
		}
	}

        if(debug)
                cprintf("Enqueued Process in Active List\n");
}

void
enqueueX_env(struct Env *e, struct prio_array_t *array)		// For enqueuing env in expired lists
{
	if(debug)
                cprintf("Entering enqueue in expired list for env_id [%08x]\n", e->env_id);
	
	int32_t d_priority;
	
	update_sched_entity(e);

	d_priority = e->env_se.dyna_priority - 100;

        // If the list is empty, we have to set the bit to 1 for the priority queue
	if(LIST_EMPTY(array->pqueue + d_priority))
	{
		set_bit(d_priority,array->bitmap);
		LIST_INSERT_HEAD(array->pqueue + d_priority,e,env_rqlink);
	}
	else
	{
		struct Env *env;
		int i = 0,j = 0;
		
		// Finds the index of last element in the list
		LIST_FOREACH(env,array->pqueue+d_priority,env_rqlink){
			i++;
		}
	
		//Insert after the last elemnt in the list
		LIST_FOREACH(env,array->pqueue+d_priority,env_rqlink){
			j++;
			if(j == i)
				LIST_INSERT_AFTER(env,e,env_rqlink);
		}
	}

        if(debug)
		cprintf("Enqueue Process in expired list\n");
}


void
dequeue_env(struct Env *e, struct prio_array_t *array)
{
	if(debug)
                cprintf("Entering dequeue for env_id [%08x]\n", e->env_id);

	int32_t d_priority;
	d_priority = e->env_se.dyna_priority - 100;
	LIST_REMOVE(e,env_rqlink);
	
	// If the list is empty, we need to set the bit to 0 for the priority queue
	if(LIST_EMPTY(array->pqueue + d_priority))
	{	
		clear_bit(d_priority,array->bitmap);
	}

	// Sets the timestamp for calculating waiting time
	e->env_se.timestamp = global_clock.real_tick;

	if(debug)
                cprintf("Dequeue done, now checking if lists need to be swapped\n");
	
	
}

void
swap_full_lists()
{
	if(debug)
	{
                cprintf("Entering swap_full_lists function\n");
		print_bitmap();
	}
	if(is_active_bitmap_empty() == 1)
	{	
		if(debug)
                        cprintf("Active List empty\n");

		if(is_expired_bitmap_empty() == 0)
		{
			if(debug)
                        	cprintf("Expired List NOT empty\n");

			//Swap Lists
			struct prio_array_t * temp;
			temp = rq.active;
			rq.active = rq.expired;
			rq.expired = temp;

			if(debug)
                        {	
			        cprintf("===================================Lists swapped==============================================\n");
				print_bitmap();
			}
		}
	}
}

void
swap_lists(uint32_t d_priority)
{
	if(debug)
		cprintf("Entering swap_lists function\n");
	
	// If the list is empty, we need to set the bit to 0 for the priority queue
	if(LIST_EMPTY(rq.active->pqueue + d_priority))
	{
		if(debug)
	        	cprintf("Active List empty\n");

		// Checks if expired list is empty or not
		if(LIST_EMPTY(rq.expired->pqueue + d_priority) != 0)
		{	
			if(debug)
                	        cprintf("Expired List NOT empty\n");
		
			// If it is not empty, swap active and expired arrays
			struct Env_list temp;

			temp = rq.active->pqueue[d_priority];
			rq.active->pqueue[d_priority] = rq.expired->pqueue[d_priority];
			rq.expired->pqueue[d_priority] = temp;

			set_bit(d_priority,rq.active->bitmap);

			if(debug)
				cprintf("Lists swapped\n");
		}
	}
}

void
print_bitmap()
{
	int i;
	
	for(i=0;i<5;i++)
	        cprintf("%08b ",rq.arrays[0].bitmap[i]);
        cprintf("\t ACTIVE \n");

        for(i=0;i<5;i++)
                cprintf("%08b ",rq.arrays[1].bitmap[i]);
        cprintf("\t EXPIRED \n");
}

void
clear_bit(uint32_t d_priority, uint8_t *bitmap)
{
	int index = d_priority/8;
	int bit = d_priority % 8;

	uint8_t mask = 0x80 >> bit;
	bitmap[index] &= ~mask;
	
	if(debug)
	{
		cprintf("Bit cleared\n");
		print_bitmap();
	}
}

void
set_bit(uint32_t d_priority, uint8_t *bitmap)
{	
	int32_t index = d_priority/8;
	int32_t bit = d_priority % 8;

	uint32_t mask = 0x80 >> bit;
	bitmap[index] |= mask;

	if(debug)
	{
		cprintf("Bit Set\n");
		print_bitmap();
	}
}

//Checks if bitmap of active queue is empty
int
is_active_bitmap_empty()
{
	int i;
	for(i=0;i<=4;i++)
	{
		if(rq.active->bitmap[i] != 0)
			return 0;
	}
	
	if(debug)
		cprintf("Active queue bitmap is EMPTY\n");
	
	return 1;
}

int
is_expired_bitmap_empty()
{
        int i;
        for(i=0;i<=4;i++)
        {
                if(rq.expired->bitmap[i] != 0)
                        return 0;
        }

        if(debug)
                cprintf("Expired queue bitmap is EMPTY\n");

        return 1;
}

int
get_max_bit()
{
	int i,k,pqueue;
	for(i=0;i<=4;i++)
	{
		uint32_t temp;
		for(k=0;k<=7;k++)
		{
			temp = 0x80 >> k;
			if(rq.active->bitmap[i] & temp)			
			{
				pqueue = (i*8)+k;
				return pqueue; 				
			}
		}
	}
	return -1;
}

uint32_t
env2envid(struct Env *env)
{
	struct Env *e;
	int i;
	//Find index in envs of curenv
	for(i = 0; i < NENV; i++)
	{
		e = &envs[i];
		if(e == env)
			return i;
	}
	return -1;

}

void 
sched_o_yield(void)
{
	//Increment real_tick_used of global clock
        global_clock.real_tick++;

	//cprintf("Real ticks: %uul\n",global_clock.real_tick);


	int i, env_index = 1024;
	struct Env *e = NULL;
	
	//Find index in envs of curenv
	env_index = env2envid(curenv);
	
	//Decrementing timeslice for current process as it has run once
	if(env_index > -1 && env_index < 1024)
		envs[env_index].env_se.timeslice--;
	else
		env_index = 1024;

	//cprintf("Entering sched_fair_yield envID : %d , timeslice_remaining %d \n", env_index, envs[env_index].env_se.timeslice);

	i = env_index;
	
	if(envs[i].env_se.timeslice == 0 && curenv != NULL && curenv != &envs[0])	// If the current process timeslice expires
	{
		// Remove the process from the active runqueue	
		dequeue_env(&envs[i],rq.active);
		//Cannot call swap_lists inside dequeue bcoz 
			//even here we'll swap the list and put the env in expired again which is not right
	
		if(debug)
			cprintf("Finished timslice in queue: %d\n",envs[i].env_se.dyna_priority);

		// Refills the timeslice
		task_timeslice(&envs[i]);

		// Insert the process in either expired or active runqueue depending on interactive nature
		// rahulrahul
		if(envs[i].env_status == ENV_RUNNABLE)
		{
		  if(task_interactive(&envs[i]) == 0)
                  {			
			//If not interactive, add to expired
                        enqueueX_env(&envs[i],rq.expired);

			if(debug)
				cprintf("Re-enqueued in expired queue: %d\n",envs[i].env_se.dyna_priority);
                  }
                  else //if(task_interactive(&envs[i]) == 1)
                  {	
			//If interactive, add to active
                        if(debug)
				cprintf("\nEnv %d classified as INTERACTIVE\n", i);	
			enqueue_env(&envs[i],rq.active);
			
			if(debug)
				cprintf("Re-enqueued in active runqueue : %d\n",envs[i].env_se.dyna_priority);
                  }
		}	
/*		
		//Checking if solution works when always put in expired queue
		if(envs[i].env_status == ENV_RUNNABLE)
		{
			enqueueX_env(&envs[i],rq.expired);

                	if(debug)
                		cprintf("Re-enqueued in expired queue: %d\n",envs[i].env_se.dyna_priority);
		}
*/		
		//CHECKING IF LISTS NEED TO BE SWAPPED
		swap_full_lists();
		//swap_lists(envs[i].env_se.dyna_priority);	// For the destination list
	}

	while(1)
	{	
		uint32_t pque;
		//Select the next process to schedule
	
		pque = get_max_bit();		// Obtains the maximum active priority run queue with processes

		if(debug)
			cprintf("MAX Priority Queue: %d \n",pque + 100);	
		if(pque != -1)
		{
			e = LIST_FIRST(rq.active->pqueue + pque);		// Obtains the first element of the priority queue

			if((e->env_status != ENV_RUNNABLE))
			{
				if(debug)
                        		cprintf("Env picked NOT_RUNNABLE ID : %d status : %d \n", env2envid(e), e->env_status);
				
				dequeue_env(e,rq.active);
				//checking if lists need to be swapped
				swap_full_lists();
				continue;
			}
	
			env_index = env2envid(e);	
			break;
		}
		else
		{
			//Ideally should not enter here except for may be first run	
			//If pque is empty it means active runqueue is empty
			//Check if lists need to be swapped
			if(debug)
				cprintf("\n================= Unable to pick env to run ===================\n");
			swap_full_lists();
			env_index = -1;
			break;
		}
	}
	if(debug)
		cprintf("Status of process selected: %d dp %d timeslice %llu\n",envs[env_index].env_status,envs[env_index].env_se.dyna_priority,envs[env_index].env_se.timeslice);
	
	cprintf("******* O(1) Scheduler Selected process ENV-ID [%08x] STATIC_P: %d DYNAMIC_P %d TIMESLICE: %llu ********\n",envs[env_index].env_id,envs[env_index].env_se.stat_priority,envs[env_index].env_se.dyna_priority,envs[env_index].env_se.timeslice);

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
