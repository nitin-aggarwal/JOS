/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* The TRAPHANDLER macro defines a globally-visible function for handling
 * a trap.  It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 */ 
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(trap_divide,T_DIVIDE)
TRAPHANDLER_NOEC(trap_debug,T_DEBUG)
TRAPHANDLER_NOEC(trap_nmi,T_NMI)
TRAPHANDLER_NOEC(trap_brkpt,T_BRKPT)
TRAPHANDLER_NOEC(trap_oflow,T_OFLOW)
TRAPHANDLER_NOEC(trap_bound,T_BOUND)
TRAPHANDLER_NOEC(trap_illop,T_ILLOP)
TRAPHANDLER_NOEC(trap_device,T_DEVICE)
TRAPHANDLER(trap_dblflt,T_DBLFLT)
TRAPHANDLER(trap_tss,T_TSS)
TRAPHANDLER(trap_segnp,T_SEGNP)
TRAPHANDLER(trap_stack,T_STACK)
TRAPHANDLER(trap_gpflt,T_GPFLT)
TRAPHANDLER(trap_pgflt,T_PGFLT)
TRAPHANDLER_NOEC(trap_fperr,T_FPERR)
TRAPHANDLER_NOEC(trap_align,T_ALIGN)
TRAPHANDLER_NOEC(trap_mchk,T_MCHK)
TRAPHANDLER_NOEC(trap_simderr,T_SIMDERR)

TRAPHANDLER_NOEC(irq_0,IRQ_OFFSET)
TRAPHANDLER_NOEC(irq_1,IRQ_OFFSET+1)
TRAPHANDLER_NOEC(irq_2,IRQ_OFFSET+2)
TRAPHANDLER_NOEC(irq_3,IRQ_OFFSET+3)
TRAPHANDLER_NOEC(irq_4,IRQ_OFFSET+4)
TRAPHANDLER_NOEC(irq_5,IRQ_OFFSET+5)
TRAPHANDLER_NOEC(irq_6,IRQ_OFFSET+6)
TRAPHANDLER_NOEC(irq_7,IRQ_OFFSET+7)
TRAPHANDLER_NOEC(irq_8,IRQ_OFFSET+8)
TRAPHANDLER_NOEC(irq_9,IRQ_OFFSET+9)
TRAPHANDLER_NOEC(irq_10,IRQ_OFFSET+10)
TRAPHANDLER_NOEC(irq_11,IRQ_OFFSET+11)
TRAPHANDLER_NOEC(irq_12,IRQ_OFFSET+12)
TRAPHANDLER_NOEC(irq_13,IRQ_OFFSET+13)
TRAPHANDLER_NOEC(irq_14,IRQ_OFFSET+14)
TRAPHANDLER_NOEC(irq_15,IRQ_OFFSET+15)

TRAPHANDLER_NOEC(trap_syscall,T_SYSCALL)
TRAPHANDLER_NOEC(trap_default,T_DEFAULT)


/*
 * Lab 3: Your code here for _alltraps
 */
.globl _alltraps
_alltraps:
	push %ds
	push %es
	pushal
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	push %esp
	call trap
	popal
	pop %es
	pop %ds
	iret
