#include "ns.h"

extern union Nsipc nsipcbuf;

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

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r;
		
	while(1)
	{
		r = sys_page_alloc(0,&nsipcbuf,PTE_P|PTE_U|PTE_W);
		if(r)
		{
			cprintf("\nPage Alloc failed in input.c\n");	
			return;
		}
		r= sys_packet_recv(nsipcbuf.pkt.jp_data);
		nsipcbuf.pkt.jp_len = r;
		if(r <= 0)
		{
			sys_yield();
			//continue;
		}
		else
		{
			//cprintf("\n Count: %d\n",nsipcbuf.pkt.jp_len);
                	//hexdump("\n Output in input.c: ",nsipcbuf.pkt.jp_data,nsipcbuf.pkt.jp_len);
                	ipc_send(ns_envid, NSREQ_INPUT,&(nsipcbuf.pkt),PTE_P|PTE_U|PTE_W);
		}
		sys_page_unmap(0,&nsipcbuf);
	}

}
