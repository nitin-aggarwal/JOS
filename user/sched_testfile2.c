#include <inc/lib.h>


// Project ( Scheduler ) --- Test Case


void
umain(void)
{
	cprintf("Environment running [%08x] \n", env->env_id);
	
	int i,k;
	for(i=0;i<16;i++)
	{
		cprintf("Hexadecimal Reprsentation for %02d :  %01x\n",i,i);
		for(k=0;k<500;k++)
		{
			cprintf("");
		}
	}
}
