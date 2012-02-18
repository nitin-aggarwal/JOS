// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).
	//cprintf("Address: 0x%08x Error :0x%04x \n",addr,err);
	// LAB 4: Your code here.
	if(!(err & FEC_WR))
		{
			cprintf("Address: 0x%08x Error :0x%04x  %e \n",addr,err, err);
			panic("Fault: Not a write");
		}

	if(!(vpt[(uintptr_t)addr / PGSIZE] & PTE_COW))
		{
			cprintf("Address: 0x%08x Error :0x%04x \n",addr,err);
			cprintf("Page number : 0x%08x Page permissions : 0x%08x\n",((uintptr_t)addr/PGSIZE), vpt[(uintptr_t)addr / PGSIZE]);
			panic("Fault Page: Not Copy-on-write");
		}
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr,PGSIZE);
	r = sys_page_alloc(0,PFTEMP,PTE_P|PTE_U|PTE_W);
	if(r)
		panic("Page Allocation: %e",r);

	memmove(PFTEMP,addr,PGSIZE);
	r = sys_page_map(0,PFTEMP,0,addr,PTE_P|PTE_U|PTE_W);
	if(r)
		panic("Page Mapping Error: %e",r);

	r = sys_page_unmap(0,PFTEMP);
	if(r)
		panic("Page Unmapping Error: %e",r);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.

	pte_t pte;
	void *addr;
	int perm = PTE_USER;
	//cprintf("inside duppage \n");
	addr = (void *) (pn *PGSIZE);
	pte = vpt[pn];

	//cprintf("Pagenumber: 0x%08x \n",pn);
	if(!pte)
		panic("Invalid virtual address");
	perm &= pte;	//was pte & perm lab7
	
	if(perm & PTE_SHARE)
		return sys_page_map(0,addr,envid,addr, perm);		//check if PTE_SHARE has to be included

	if(perm & (PTE_W|PTE_COW))
	{
		perm = perm & ~PTE_W;
		perm |= PTE_COW;	// didt work even if PTE_U is removed for lab 7
		
		r = sys_page_map(0,addr,envid,addr,perm);
		if(r)
		{	
			return r;
		}
		
		return sys_page_map(0,addr,0,addr,perm);
	}

	return sys_page_map(0,addr,envid,addr,perm);
	//panic("duppage not implemented");
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	//struct Env *env;
	uint32_t i,j,r;

	set_pgfault_handler(pgfault);

	envid = sys_exofork();
	
	//cprintf("ENVID: %04x vpd: %08x\n",envid,VPD(UTOP));
	
	if(envid < 0)
		return envid;  // Invalid environemnt id
	else if(envid == 0)
	{
		env = &envs[ENVX(sys_getenvid())];
		return 0;		
	}
	
	uint32_t pagenum = 0;
	 
	for(i=0;i<VPD(UTOP);i++)
	{
		if(vpd[i] == 0)
		{
			pagenum += NPTENTRIES;
			continue;
		}
		for(j=0;j<NPTENTRIES;j++)
		{
			
			//cprintf("Page: %d \n",pagenum);
			if(vpt[pagenum] == 0)
			{
				pagenum++;
				continue;
			}
			if(pagenum != ((UXSTACKTOP - PGSIZE)/PGSIZE))
			{
				r = duppage(envid,pagenum);
				if(r)
					panic("duppage error");
			}
			pagenum++;
		}
	}
	r = sys_page_alloc(envid,(void *)(UXSTACKTOP - PGSIZE),PTE_U|PTE_P|PTE_W);
	if(r)
		return r;
	sys_env_set_pgfault_upcall(envid,env->env_pgfault_upcall);
	
	
	r = sys_env_set_status(envid,ENV_RUNNABLE);
	if(r)
		panic("Env status error");
		
	//cprintf("Fork Exited:\n");
	//panic("fork not implemented");
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
