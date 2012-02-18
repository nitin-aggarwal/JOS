#ifndef JOS_KERN_E100_H
#define JOS_KERN_E100_H

#include <inc/memlayout.h>
#include <inc/assert.h>
#include <kern/pci.h>

#define DMA_SIZE 	10	//change size to whatever required
#define MAX_PKT_SIZE 	1518
#define SCB_INT_MASK	0x0100
#define CU_CMD_TRANSMIT	0x0004		//check this
#define CU_STATUS_C	0x8000
#define CU_STATUS_OK	0x2000
#define CU_CMD_EL	0x8000
#define CU_CMD_S	0x4000
//EL, S, C, OK bits same for tcb and rfa
#define RU_STATUS_C	0x8000
#define RU_STATUS_OK	0x2000
#define RU_CMD_EL	0x8000
#define RU_CMD_S	0x4000
#define RU_CNT_EOF	0x8000
#define RU_CNT_F	0x4000

#define SCB_CU_ACTIVE	0x0080
#define SCB_CU_IDLE	0x0000
#define SCB_CU_SUSPEND	0x0040
#define SCB_LOAD_BASE 	0x0060

#define SCB_CU_START  	0x0010
#define SCB_CU_RESUME	0x0020

#define SCB_RU_READY	0x0010	   
#define SCB_RU_IDLE 	0x0000    
#define SCB_RU_SUSPEND  0x0004
#define SCB_RU_NO_RES	0x0008

#define SCB_RU_START 	0x0001   
#define SCB_RU_RESUME   0x0002
#define SCB_RU_ABORT	0x0004

extern int send_pkt(void *, size_t);
extern int recv_pkt(void *);
extern uint8_t irq_line;
extern uint32_t reg_base;
int attachNIC(struct pci_func *);
void cu_start(void);
void ru_start(void);
void reclaim_tcb(void);
void reclaim_rfa(void);

struct tcb{
	volatile uint16_t status;
	uint16_t cmd;
	uint32_t link;
	uint32_t tbd_addr;	//set to 0xFFFFFFFF - null pointer
	uint16_t tcb_size;
	uint8_t thrs;		//transmit threshold set to 0xE0
	uint8_t tbd_count;
	char pkt_data[MAX_PKT_SIZE];	//shd check & #define this limit 
}__attribute__((packed));

struct rfa{
	volatile uint16_t status;
	uint16_t cmd;
	uint32_t link;
	uint32_t reserved;
	uint16_t byte_count;
	uint16_t size_buffer;
	char pkt_data[MAX_PKT_SIZE];
}__attribute__((packed));

extern struct tcb tcb_list[DMA_SIZE];
extern struct rfa rfa_list[DMA_SIZE];

#define PADDR(kva)                                              \
({                                                              \
        physaddr_t __m_kva = (physaddr_t) (kva);                \
        if (__m_kva < KERNBASE)                                 \
                panic("PADDR called with invalid kva %08lx", __m_kva);\
        __m_kva - KERNBASE;                                     \
})

void tcb_list_init(void);
void rfa_list_init(void);

#endif	// JOS_KERN_E100_H
