/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_TRAP_H
#define JOS_KERN_TRAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/trap.h>
#include <inc/mmu.h>

/* The kernel's interrupt descriptor table */
extern struct Gatedesc idt[];

void idt_init(void);
void print_regs(struct PushRegs *regs);
void print_trapframe(struct Trapframe *tf);
void page_fault_handler(struct Trapframe *);
void backtrace(struct Trapframe *);

void trap_divide(void);
void trap_debug(void);
void trap_nmi(void);
void trap_brkpt(void);
void trap_oflow(void);
void trap_bound(void);
void trap_illop(void);
void trap_device(void);
void trap_dblflt(void);
void trap_tss(void);
void trap_segnp(void);
void trap_stack(void);
void trap_gpflt(void);
void trap_pgflt(void);
void trap_fperr(void);
void trap_align(void);
void trap_mchk(void);
void trap_simderr(void);
void trap_syscall(void);

void irq_0(void);
void irq_1(void);
void irq_2(void);
void irq_3(void);
void irq_4(void);
void irq_5(void);
void irq_6(void);
void irq_7(void);
void irq_8(void);
void irq_9(void);
void irq_10(void);
void irq_11(void);
void irq_12(void);
void irq_13(void);
void irq_14(void);
void irq_15(void);

#endif /* JOS_KERN_TRAP_H */
