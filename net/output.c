#include "ns.h"
#include <kern/e100.h>
#include <inc/lib.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	
	while(1)
	{
		ipc_recv(NULL, &nsipcbuf, NULL);
		if(env->env_ipc_from != ns_envid)
			continue;
		if(env->env_ipc_value != NSREQ_OUTPUT)
			continue;
		sys_packet_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);	
	}
}
