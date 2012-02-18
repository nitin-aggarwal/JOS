/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>
#include <kern/e100.h>

#include <kern/sched_cfs.h>
#include <kern/sched_o.h>

#define debug 0

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	
	// LAB 3: Your code here.
	user_mem_assert(curenv,s,len,PTE_U);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	struct Env *e;
	envid_t i;
	i  = env_alloc(&e,curenv->env_id);

	if(debug)	
		cprintf("i = %08x env_id: %d\n",i,e->env_id);
	
	if(i == -E_NO_FREE_ENV)
		return i;	//cprintf("No free environmnets available \n");
	else if (i == -E_NO_MEM)
		return i; //cprintf("No Memory available for environment \n");
	else if (i == 0)
	{
		e->env_status = ENV_NOT_RUNNABLE;

		if(SCHED_CLASS == 3)			// Dequeue the process as it becomes NOT_RUNNABLE
		{
			if(debug)
                                cprintf("sys_exofork NOT_RUNNABLE : env : [%08x] \n", e->env_id);

			dequeue_env(e,rq.active);
			//check if lists need to be swapped
			swap_full_lists();
		}
	
		e->env_tf = curenv->env_tf;	
		e->env_tf.tf_regs.reg_eax = 0;
		return e->env_id;
	}
	else
	{return i;}
	//panic("sys_exofork not implemented");
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	struct Env *e;
	int i = envid2env(envid, &e, 1);
	if(i == -E_BAD_ENV)
		return i;
	if(i == 0)
	{
		if(status == ENV_RUNNABLE || status == ENV_NOT_RUNNABLE)
			e->env_status = status;
		else
			return -E_INVAL;
	}
	
	//Project
	if(SCHED_CLASS == 3)		//O(1)
	{ 
		if(e->env_status == ENV_RUNNABLE)
	 	{
			if(debug)
                        	cprintf("sys_env_set_status RUNNABLE : env : [%08x] \n", e->env_id);
		//dequeueing and enqueueing just to make sure env is in correct priority queue
			dequeue_env(e,rq.active);
			enqueue_env(e,rq.active);
	 	}
		else if(e->env_status == ENV_NOT_RUNNABLE)
		{
			if(debug)
                                cprintf("sys_env_set_status NOT_RUNNABLE : env : [%08x] \n", e->env_id);

			dequeue_env(e,rq.active);
			//check if lists need to be swapped
			swap_full_lists();
		}
	}

	return i;
	//panic("sys_env_set_status not implemented");
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *env;
	int r;

	r = envid2env(envid,&env,1);
	if(r)
		return r;
	env->env_tf = *tf;
	env->env_tf.tf_eflags |= FL_IF;
	env->env_tf.tf_eflags &= ~(FL_IOPL_3);
	
	return 0;
	//panic("sys_env_set_trapframe not implemented");
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.	
	struct Env *e;
	int i = envid2env(envid, &e, 1);
	if(i == -E_BAD_ENV)
		return i;
	if(i == 0)
	{
		e->env_pgfault_upcall = func;
		return i;
	}
	return i;
	//panic("sys_env_set_pgfault_upcall not implemented");
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_USER in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	struct Page *p;	
	struct Env *e;

	//check for environment
	int i = envid2env(envid, &e, 1);
	
	if(i == -E_BAD_ENV)
		return i;
	
	i = page_alloc(&p);
	
	if(i == -E_NO_MEM)
		return i;
	else if(i == 0)
	{
		//check for perm
		int temp;
		temp = PTE_U|PTE_P|PTE_AVAIL|PTE_W;
		if(((perm & PTE_U) == 0) || ((perm & PTE_P) == 0))
			return -E_INVAL;
	
		if(perm & ~temp)
			return -E_INVAL;
		
		//check for virtual address
		if(((uintptr_t)va >= UTOP) || (ROUNDUP(va,PGSIZE) != va))
			return -E_INVAL;

		i = page_insert(e->env_pgdir,p,va,perm);
		if(i == -E_NO_MEM)
		{
			page_free(p);
			return i;
		}
			
		memset(page2kva(p), 0, PGSIZE);
	}
	return i;
	//panic("sys_page_alloc not implemented");
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E	_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	struct Env *src,*dest;
	struct Page *p;
	pte_t *pte;
	int pde;

	//check for environments
	int i = envid2env(srcenvid, &src,0);
	int j = envid2env(dstenvid, &dest,0);
	if(i == -E_BAD_ENV || j == -E_BAD_ENV)
		return i;
	else
	{
		//check for virtual address
		if(((uint32_t ) srcva) >= UTOP || ((uint32_t ) dstva) >= UTOP)
			return -E_INVAL;
		if((ROUNDUP(srcva,PGSIZE)) != srcva || (ROUNDUP(dstva,PGSIZE)) != dstva)
			return -E_INVAL;
	
			
		//check for perm
		int temp;
		temp = PTE_U|PTE_P|PTE_AVAIL|PTE_W;
		if(((perm & PTE_U) == 0) || ((perm & PTE_P) == 0))
			return -E_INVAL;

		//rahul
		//Removed check for Lab 5 ex2, PTE_D had to be cleared after flush_block	
		//if(perm & ~temp)
		//	return -E_INVAL;

		p = page_lookup(src->env_pgdir, srcva, &pte);
		
		if(!p)
			return -E_INVAL;

		pde =src->env_pgdir[PDX(srcva)];
		if(perm & PTE_W)
		{
			if(((*pte & PTE_W) == 0) || ((pde & PTE_W) == 0)) 
				return -E_INVAL;
		}

		i = page_insert(dest->env_pgdir,p,dstva,perm);
		if(i == -E_NO_MEM)
		{
			page_free(p);
			return i;
		}
		else if(i == 0)
			return 0;
	}
	return  0;
	//panic("sys_page_map not implemented");
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	
	struct Env *e;

	//check for environment
	int i = envid2env(envid, &e, 1);
	if(i == -E_BAD_ENV)
		return i;
	if(((uint32_t )va) >= UTOP || (ROUNDUP(va, PGSIZE)!= va))
		return -E_INVAL;
	page_remove(e->env_pgdir, va);
	return 0;
	//panic("sys_page_unmap not implemented");
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	int i;
	struct Page *p;
	struct Env *dest,*src;
	pte_t *pte,pde;
	i = envid2env(0,&src,0);
	if(i < 0)       
                return -E_BAD_ENV;
	i = envid2env(envid,&dest,0);
	//cprintf("value of envid in sys_send is %d \n",i);
	if(i < 0)
		return -E_BAD_ENV;
	if(dest->env_ipc_recving == 0)
		return -E_IPC_NOT_RECV;

	if(debug)
		cprintf("srcva: %08x\n",srcva);
	
	if(srcva)
	{
		if((uint32_t)srcva >= UTOP)
			return -E_INVAL;
		//check for virtual address
		if(((uint32_t)srcva % PGSIZE) != 0)
			return -E_INVAL;
		//check for perm
		int temp;
		temp = PTE_U|PTE_P|PTE_AVAIL|PTE_W;
		if(((perm & PTE_U) == 0) || ((perm & PTE_P) == 0))
			return -E_INVAL;
		if(perm & ~temp)
			return -E_INVAL;
	}
	else
	{
		
		dest->env_ipc_recving = 0;
		dest->env_ipc_from = curenv->env_id;
        	dest->env_ipc_value = value;
		dest->env_ipc_perm = 0;
		dest->env_status = ENV_RUNNABLE;
		
		//Project-- Enqueue the runnable process in a priority queue
		if(SCHED_CLASS == 3)                    //O(1)
			enqueue_env(dest,rq.active);

		return 0;
	}
	dest->env_ipc_recving = 0;
	dest->env_ipc_perm = 0;
	if((uint32_t)(dest->env_ipc_dstva) != -1)
	{
		i = sys_page_map(src->env_id,srcva,dest->env_id,dest->env_ipc_dstva,perm);
		if(i < 0)
			return i;	
		dest->env_ipc_perm = perm;	
	}
	dest->env_ipc_from = curenv->env_id;
	dest->env_ipc_value = value;
	dest->env_status = ENV_RUNNABLE;
	
	//Project
	if(SCHED_CLASS == 3)			//O(1)
		enqueue_env(dest,rq.active);

	
	return 0;
	//panic("sys_ipc_try_send not implemented");
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	int i;
	struct Env *env;
	
	if(debug)
		cprintf("dstva %08x\n",dstva);

	if(((uint32_t )dstva < UTOP) && (((uint32_t )dstva % PGSIZE) != 0))
		return -E_INVAL;

	i = envid2env(0,&env,0);
	if(i)
		return i;

	env->env_ipc_dstva = dstva;	
	env->env_ipc_recving = 1;
	env->env_status = ENV_NOT_RUNNABLE;	
	
	//Project - dequeue the not runnable process form the priority queue
	if(SCHED_CLASS == 3)
	{
		if(debug)
                	cprintf("sys_ipc_recv NOT RUNNABLE : env : [%08x] \n", env->env_id);
		dequeue_env(env,rq.active);
		
		//check if lists have to be swapped
		swap_full_lists();
	}
	
		
	//panic("sys_ipc_recv not implemented");
	return 0;
}

// Return the current time.
static int
sys_time_msec(void) 
{
	// LAB 6: Your code here.
	return time_msec();
	//panic("sys_time_msec not implemented");
}

// Return
static int
sys_packet_send(void *data,size_t datalen)
{
	return send_pkt(data, datalen);
}

static int
sys_packet_recv(void *buf)
{
	int r;
	return recv_pkt(buf);
	/*if(r == -1)
	{
		//curenv->env_status = ENV_NOT_RUNNABLE;
		sched_yield();
	}
	return 0;*/
}

// Project
// returns 0 on success
// Sets priority and min_vruntime based on priority
static int
sys_set_priority(envid_t envid, int32_t priority)
{
	struct Env *e;
	int r;
	r = envid2env(envid, &e, 0);
	if(r < 0)
		return r;
	if(priority > MAX_PRIORITY)
		priority = MAX_PRIORITY;
	else if(priority < MIN_PRIORITY)
		priority = MIN_PRIORITY;

	e->env_se.priority = priority;
	e->env_se.timeslice = MIN_TIMESLICE * (priority - MIN_PRIORITY + 1);
	
	e->env_se.vruntime = global_clock.global_tick - 1;

	return 0;		
}
// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	switch(syscallno)
	{
		case SYS_cputs:
			sys_cputs((const char *) a1,a2);
			return 0;
		case SYS_cgetc:
			return sys_cgetc();
		case SYS_getenvid:
			return sys_getenvid();
		case SYS_env_destroy:
			return sys_env_destroy((envid_t) a1);
		case SYS_yield:
			sys_yield();
			return 0;			
		case SYS_exofork:
			return sys_exofork();
		case SYS_env_set_status:
			return sys_env_set_status((envid_t)a1, a2);
		case SYS_page_alloc:
			return sys_page_alloc((envid_t) a1, (void *) a2, a3);
		case SYS_page_map:
			return sys_page_map((envid_t) a1, (void *) a2, (envid_t) a3, (void *) a4, a5);
		case SYS_page_unmap:
			return sys_page_unmap((envid_t) a1, (void *) a2);
		case SYS_env_set_pgfault_upcall:
			return sys_env_set_pgfault_upcall((envid_t)a1,(void *)a2);
		case SYS_ipc_try_send:
			return sys_ipc_try_send((envid_t)a1,(uint32_t)a2,(void *)a3,(unsigned)a4);
		case SYS_ipc_recv:
			return sys_ipc_recv((void *)a1);
		case SYS_env_set_trapframe:
			return sys_env_set_trapframe((envid_t)a1,(struct Trapframe *)a2);
		case SYS_time_msec:
			return sys_time_msec();
		case SYS_packet_send:
			return sys_packet_send((void *)a1, (size_t) a2);
		case SYS_packet_recv:
			return sys_packet_recv((void *)a1);
		case SYS_set_priority:
			return sys_set_priority((envid_t) a1, (int32_t) a2);
		default:
			return -E_INVAL;
	}
	//panic("syscall not implemented");
}

