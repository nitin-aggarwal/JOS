// LAB 6: Your driver code here

#include<kern/pci.h>
#include<inc/lib.h>
#include<inc/x86.h>
#include<kern/e100.h>


struct tcb tcb_list[DMA_SIZE];
struct rfa rfa_list[DMA_SIZE];

uint8_t irq_line;
uint32_t reg_base;

uint8_t free_tcb;
uint8_t free_rfa;

static void
hexdump(const char *prefix, const void *data, int len)
{
        int i;
        char buf[80];
        char *end = buf + sizeof(buf);
        char *out = NULL;
        for (i = 0; i < len; i++) {
                if (i % 16 == 0)
                        out = buf + snprintf(buf, end - buf,
                                             "%s%04x   ", prefix, i);
                out += snprintf(out, end - out, "%02x", ((uint8_t*)data)[i]);
                if (i % 16 == 15 || i == len - 1)
                        cprintf("%.*s\n", out - buf, buf);
                if (i % 2 == 1)
                        *(out++) = ' ';
                if (i % 16 == 7)
                        *(out++) = ' ';
        }
}

int attachNIC(struct pci_func *pcifn)
{
	int i;

	pci_func_enable(pcifn);

	irq_line = pcifn->irq_line;
	reg_base = pcifn->reg_base[1];

	assert(reg_base < 65536);	

	//Software-Reset the NIC by writing
	//opcode 0000 in Port
	outl(reg_base + 0x8,0x0);

	// Deliberate Delay after a Reset 
	for(i=0;i<8;i++)
		inb(0x84);

	// Initializing the default fields and links
	// in Transmit Command Block
	tcb_list_init();
		
	// Initializing the default fields and links
	// in Receive Frame
	rfa_list_init();

	ru_start();

	uint32_t temp_nut = inl(reg_base);
	//cprintf("\nEnd attachNIC SCB:CS 0x%08x\n", temp_nut);
	return 0;
}

void 
tcb_list_init(void)
{
	int i;
	// Clearing out the space allocated
	memset(tcb_list,0,sizeof(tcb_list));
	//cprintf("size: %d\n",sizeof(tcb_list));

	for(i = 0; i<DMA_SIZE; i++)
	{	
		//To make array circular use %DMA_SIZE
		tcb_list[i].link = PADDR(&tcb_list[(i+1) % DMA_SIZE]);
		
		tcb_list[i].tbd_addr = 0xffffffff;
		tcb_list[i].tbd_count = 0;
		tcb_list[i].thrs = 0xe0;
		tcb_list[i].tcb_size = 0;
	}
	free_tcb = 0;
}

void 
rfa_list_init(void)
{
	int i;
	// Clearing out the space allocated
	memset(rfa_list,0,sizeof(rfa_list));

	for(i = 0; i<DMA_SIZE; i++)
	{	
		//To make array circular use %DMA_SIZE
		rfa_list[i].link = PADDR(&rfa_list[(i+1) % DMA_SIZE]);
		
		rfa_list[i].reserved = 0xffffffff;
		rfa_list[i].size_buffer = MAX_PKT_SIZE;
		rfa_list[i].cmd =0;
		rfa_list[i].status = 0;
		rfa_list[i].byte_count = 0;
	}
	free_rfa = 0;
	rfa_list[DMA_SIZE - 1].cmd = RU_CMD_EL;		
}

void
cu_start(void)
{
	// Move pointer to first CB in CBL
	// in SCB General Pointer
	outl(reg_base + 0x4,PADDR(&tcb_list[0]));
	
	// Mask interrupts
	outb(reg_base + 0x3, 0xFD);   //SCB_INT_MASK  previusly

	// Move Opcode for cu-start in CommandWord
	outb(reg_base + 0x2, (inb(reg_base + 0x2) | SCB_CU_START));

	uint32_t temp_start = inl(reg_base);
	//cprintf("\nEnd of CU_START SCB:CS 0x%08x\n", temp_start);
}

void
ru_start(void)
{	
	// Move pointer to first RFA in RF list
	// in General Pointer
	outl(reg_base + 0x4,PADDR(&rfa_list[0]));
	
	// Mask interrupts
	outb(reg_base + 0x3, 0xFD);   //SCB_INT_MASK  previusly
	
	// Move Opcode for ru-start in CommandWord
	outb(reg_base + 0x2, (inb(reg_base + 0x2) | SCB_RU_START));

	uint32_t temp_start = inl(reg_base);
	//cprintf("\nEnd of RU_START SCB:CS 0x%08x\n", temp_start);
}

void
reclaim_tcb(void)
{
	int i;
	for(i=0; i<DMA_SIZE; i++)
	{
		if((tcb_list[i].status & CU_STATUS_C) != 0)
		{
			//reclaiming even if error occured and OK bit is not 1 
			tcb_list[i].cmd = 0;
			tcb_list[i].status = 0;
			tcb_list[i].tcb_size = 0;
			tcb_list[i].tbd_count = 0;
			memset(tcb_list[i].pkt_data,0,1518);
		}
	}
	//cprintf("\nEnd of tcb reclaim\n");
}
	

void
reclaim_rfa(void)
{
	int i;
	i = free_rfa;
	rfa_list[i].cmd = 0;
	rfa_list[i].status = 0;
	rfa_list[i].byte_count = 0;
	//rfa_list[i].size_buffer = MAX_PKT_SIZE;
	rfa_list[(i-1+DMA_SIZE)%DMA_SIZE].cmd &= ~RU_CMD_EL;		
	rfa_list[i].cmd = RU_CMD_EL;
	memset(rfa_list[i].pkt_data,0,1518);
	//cprintf("\nEnd of rfa reclaim\n");
}


int
send_pkt(void *va, size_t len)
{
	int i;
	uint8_t scb_status;

	//Reclaiming all the free DMA rings	
	reclaim_tcb();

	if(len > MAX_PKT_SIZE)
		len = MAX_PKT_SIZE;

	//cprintf("\nStart of send pkt CBL cmd %x status %x SBC:ST 0x%08x\n", tcb_list[free_tcb].cmd , tcb_list[free_tcb].status,inl(reg_base));
	
	if(tcb_list[free_tcb].cmd == 0)
	{
		//Move stuff into buffer
		tcb_list[free_tcb].tcb_size = len;
		memmove(tcb_list[free_tcb].pkt_data, (const void *) va, len);

		//Transmit Command with S bit set
		//When S bit is set CU will be suspended after execution of this TCB 
		tcb_list[free_tcb].cmd = CU_CMD_TRANSMIT | CU_CMD_S ;	
	}
	else
	{
		//cprintf("\nTCB Buffer full, so packet dropped\n");
		return -1;
	}

	scb_status = inb(reg_base);
	//cprintf("\nBefore transmit CBL CMD: 0x%04x ST: 0x%04x SCB:ST 0x%08x\n", tcb_list[free_tcb].cmd,tcb_list[free_tcb].status,inl(reg_base));
	
	if((scb_status & 0xC0) == 0)
	{
		//idle
		//cprintf("\nIn Idle state\n");
		cu_start();
	}
	else if(scb_status & SCB_CU_SUSPEND)
	{
		//suspended
		//cprintf("\nIn suspended state\n");
		outb(reg_base + 0x2, (inb(reg_base + 0x2) | SCB_CU_RESUME));
	}
	//cprintf("\nAfter transmit CBL CMD: 0x%04x ST: 0x%04x SCB:CS 0x%08x\n", tcb_list[free_tcb].cmd,tcb_list[free_tcb].status,inl(reg_base));
	
	free_tcb = (free_tcb+1)%DMA_SIZE;
	//cprintf("\nExiting send_pkt\n");
	return 0;	
}


int
recv_pkt(void *buf)
{
	int count;
	uint16_t scb_cmdstatus = inl(reg_base);
	
	//cprintf("\nStart of rcv pkt RFA cmd %x status %x SCB:ST 0x%08x\n", rfa_list[free_rfa].cmd , rfa_list[free_rfa].status,scb_cmdstatus);
	if(rfa_list[free_rfa].status & RU_STATUS_C)
	{
                int i;
		
		//Check for F flag in Actual Count
		while(!(rfa_list[free_rfa].byte_count & 0xC000));
		
		count = rfa_list[free_rfa].byte_count & 0x3FFF;
		if(count > MAX_PKT_SIZE)
			count = MAX_PKT_SIZE;
		
		//cprintf("\n Count: %d\n",count);
		//hexdump("\noutput in e100: ",rfa_list[free_rfa].pkt_data,count);

		//Move stuff into buffer
		memset(buf,0,4096);
		memmove(buf,rfa_list[free_rfa].pkt_data, count);

		//cprintf("\nBefore receive  RFA-1  %d CMD: 0x%04x ST: 0x%04x SCB:ST 0x%08x\n",(free_rfa-1+DMA_SIZE)%DMA_SIZE, 
		//	rfa_list[(free_rfa-1+DMA_SIZE)%DMA_SIZE].cmd,rfa_list[(free_rfa-1+DMA_SIZE)%DMA_SIZE].status,inl(reg_base));		
		//cprintf("\nBefore receive  RFA %d CMD: 0x%04x ST: 0x%04x SCB:ST 0x%08x\n", free_rfa,rfa_list[free_rfa].cmd,rfa_list[free_rfa].status,inl(reg_base));

		reclaim_rfa();
		//cprintf("\nhere\n");

		//cprintf("\nAfter receive  RFA %d CMD: 0x%04x ST: 0x%04x SCB:ST 0x%08x\n", free_rfa,rfa_list[free_rfa].cmd,rfa_list[free_rfa].status,inl(reg_base));
		//cprintf("\nAfter receive  RFA-1  %d CMD: 0x%04x ST: 0x%04x SCB:ST 0x%08x\n",(free_rfa-1+DMA_SIZE)%DMA_SIZE, 
		//	rfa_list[(free_rfa-1+DMA_SIZE)%DMA_SIZE].cmd,rfa_list[(free_rfa-1+DMA_SIZE)%DMA_SIZE].status,inl(reg_base));

		free_rfa = (free_rfa + 1)%DMA_SIZE;
		return count;
	}
	//cprintf("\nExiting recv_pkt\n");	
	return 0;
}	
