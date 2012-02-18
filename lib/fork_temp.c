// implement fork from user space
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
	perm &= pte;
	
	if(perm & PTE_SHARE)
		return sys_page_map(0,addr,envid,addr,perm);	//check if PTE_SHARE needs to be added

	if(perm & (PTE_W|PTE_COW))
	{
		perm = perm & ~PTE_W;
		perm |= PTE_COW|PTE_U;
		
		r = sys_page_map(0,addr,envid,addr,perm);
		if(r)
			return r;
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
		panic("Invalid process ID");
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
