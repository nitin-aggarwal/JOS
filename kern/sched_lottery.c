#include <inc/env.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/sched.h>
#include <kern/sched_lottery.h>
#include <kern/sched_o.h>

#define debug  0
 
uint32_t ticket_list[MAX_TOTAL_TICKETS + 1]; 
uint32_t ticket_count_issued = 0;
	
int seed1 = 45;
int seed2 = 99;

void
ticket_list_init(void)
{
	int i;
	
	// Lottery Tickets numbered from 1 to MAX_TOTAL_TICKETS
	for(i=1; i <= MAX_TOTAL_TICKETS; i++)
		ticket_list[i] = -1;
}
int
rand(uint32_t limit)
{
	int i, rand, xy;
      
	//Making sure % by zero does not occur 
	if(limit == 0)
		limit = 1; 

      	seed1 = 36969 * (seed1 & 65535) + (seed1 >> 16);
	seed2 = 18000 * (seed2 & 65535) + (seed2 >> 16);
        rand = (seed1 << 16) + seed2;
        rand = rand % limit;
        if(rand < 0)
       		rand = -rand;
	rand = rand + 1;
        //cprintf("Rand value is : %d\n", rand);

        return rand;
}

void
ticket_alloc(struct Env *e)
{
	int i,env_index,tickets;
	int k =1;

	env_index = env2envid(e);
	tickets = envs[env_index].env_se.ticket_count;
	for(i=0;i<tickets;i++)
	{
		// Lottery tickets numbered from 1 to MAX_TOTAL_TICKETS
		for(;k<=MAX_TOTAL_TICKETS;k++)
		{
			// Select a free lottery ticket
			if(ticket_list[k] == -1)
			{
				envs[env_index].env_se.tickets[i] = k;
				ticket_list[k] = env_index;
				break;
			}
		}
	}

	// Update the global tickets issued count
	ticket_count_issued += tickets;
		
}

void
ticket_free(struct Env *e)
{
	int env_index,tickets;
	int i,ticket_no;
	
	env_index = env2envid(e);
	tickets = envs[env_index].env_se.ticket_count;

	for(i=0;i<tickets;i++)
	{
		// Free the lottery ticket issued
		ticket_no = envs[env_index].env_se.tickets[i];
		
		ticket_list[ticket_no] = -1;
		envs[env_index].env_se.tickets[i] = 0;
	}

	// Update the global tickets issued count
	ticket_count_issued -= tickets;

	// Reset the process tickets issued count
	envs[env_index].env_se.ticket_count = 0;
	
}
void
sched_lottery_yield(void)
{

        int i, k, random, temp_index = 0, env_index = -1;

	// Count of number of tickets of runnable process
        int runnable_count = 0;
	
	// Count the number of tickets of all runnable processes except idle process
	for(i=1; i<NENV; i++)
	{
		if(envs[i].env_status == ENV_RUNNABLE)
		{
			runnable_count += envs[i].env_se.ticket_count;
		}			
	}	

	random = rand(runnable_count);	
	k = 0;
	
	// Select the lucky lottery process
	for(i=0; i<MAX_TOTAL_TICKETS; i++)
	{	
		//getting index of env from ticket_list
		temp_index = ticket_list[i];
		
		if(temp_index > 0 && temp_index < NENV)
		{	
		   if(envs[temp_index].env_status == ENV_RUNNABLE)
		   {
			//increment the counter k whenever we find a env runnable
			//so effectively we start the nth runnable env (n given by random)
			k++;
			if(random == k)
			{
				env_index = temp_index;
				break;
			}
		   }
		}
	}

	if(debug)
		cprintf("Env index selected to run: %08x\n",envs[env_index].env_id);

	cprintf("****** Lottery Scheduler selected process ENV-ID [%08x] TICKETS: %d *******\n",envs[env_index].env_id,envs[env_index].env_se.ticket_count);
        // Run the special idle environment when nothing else is runnable.
        if(env_index > 0)
                env_run(&envs[env_index]);    
	else if(envs[0].env_status == ENV_RUNNABLE)
                env_run(&envs[0]);
        else {
                cprintf("Destroyed all environments - nothing more to do!\n");
                while (1)
                        monitor(NULL);
        }
}
