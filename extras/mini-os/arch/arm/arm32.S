#define PHYS_START (0x80008000)

.section .text

.globl _start
_start:
	@ zImage header
.rept   8
        mov     r0, r0
.endr
        b       reset
        .word   0x016f2818      @ Magic numbers to help the loader
        .word   0		@ zImage start address
        .word   _edata - _start @ zImage end address (excludes bss section)
	@ end of zImage header

@ Called at boot time. Sets up MMU, exception vectors and stack, and then calls C setup() function.
@ => r2 -> DTB
@ <= never returns
reset:
	@ Fill in the top-level translation table (at page_dir).
	@ Populate the whole pagedir with 1MB section descriptors.
	@ TEX[2:0] C B = 001 1 1 (outer and inner write-back, write-allocate)
	ldr	r0, =(0x2 +  		/* Section entry */ \
		      0xc +  		/* C B */ \
		      (3 << 10) + 	/* Read/write */ \
		      (1 << 12) +	/* TEX */ \
		      (1 << 16) +	/* Sharable */ \
		      (1<<19))		/* Non-secure */
	ldr	r1, =page_dir
	add	r3, r1, #4*4*1024	@ Limit (4 GB address space, 4 byte entries)

1:
	str	r0, [r1],#4		@ write the section entry
	add	r0, r0, #1 << 20 	@ next physical page
	cmp	r1, r3
	bne	1b

	@ Tell the system where our new table is located.
	ldr	r3, =page_dir
	mcr	p15, 0, r3, c2, c0, 0	@ set ttbr0

	@ Set access permission for domains
	@ Domains are deprecated, but we have to configure them anyway.
	@ We mark every page as being domain 0 and set domain 0 to "client mode"
	@ (client mode = use access flags in page table).
	mov	r0, #1			@ 1 = client
	mcr	p15, 0, r0, c3, c0, 0	@ DACR

	@ Invalidate TLB
	mcr	p15, 0, r1, c8, c7, 0	@ TLBIALL

	@ Enable MMU / SCTLR
	mrc	p15, 0, r1, c1, c0, 0	@ SCTLR
	orr	r1, r1, #0x5		@ (dcache, MMU)
	orr	r1, r1, #3 << 11	@ (icache, branch prediction)
	mcr	p15, 0, r1, c1, c0, 0	@ SCTLR
	isb

	@ Set VBAR -> exception_vector_table
	@ SCTLR.V = 0
	adr	r0, exception_vector_table
	mcr	p15, 0, r0, c12, c0, 0

	@ Initialise 16 KB stack
	ldr	sp, =stack_end

	mov	r0, r2			@ C wants the DTB pointer in r0
	b	arch_init

.pushsection .data
.align	14
page_dir:
	.fill (4*1024), 4, 0x0

.align 12
.globl shared_info_page
shared_info_page:
	.fill (1024), 4, 0x0

.align 3
.globl stack
stack:
	.fill (4*1024), 4, 0x0
stack_end:

.align 3
irqstack:
	.fill (1024), 4, 0x0
irqstack_end:
.popsection

@ exception base address
.align 5
.globl exception_vector_table
@ Note: remember to call CLREX if returning from an exception:
@ "The architecture enables the local monitor to treat any exclusive store as
@  matching a previous LDREX address. For this reason, use of the CLREX
@  instruction to clear an existing tag is required on context switches."
@ -- ARM Cortex-A Series Programmer’s Guide (Version: 4.0)
exception_vector_table:
	b	. @ reset
	b	. @ undefined instruction
	b	. @ supervisor call
	b	. @ prefetch call
	b	. @ prefetch abort
	b	. @ data abort
	b	irq_handler @ irq
	.word 0xe7f000f0    @ abort on FIQ

irq_handler:
	ldr	sp, =irqstack_end
	push	{r0 - r12, r14}

	ldr	r0, IRQ_handler
	cmp	r0, #0
	.word	0x07f000f0    @ undeq - panic if no handler
	blx	r0

	@ Return from IRQ
	pop	{r0 - r12, r14}
	clrex
	subs	pc, lr, #4

.globl IRQ_handler
IRQ_handler:
	.long	0x0


.globl __arch_switch_threads
@ => r0 = prev->sp
@    r1 = next->sp
@ <= returns to next thread's saved return address
__arch_switch_threads:
	stmia	r0, {sp, lr}	@ Store current sp and ip to prev's struct thread
	str	fp, [sp, #-4]	@ Store fp on the old stack

	ldmia	r1, {sp, lr}	@ Load new sp, ip from next's struct thread
	ldr	fp, [sp, #-4]	@ Restore fp from the stack

	mov	pc, lr

@ This is called if you try to divide by zero. For now, we make a supervisor call,
@ which will make us halt.
.globl raise
raise:
	svc	0

.globl arm_start_thread
arm_start_thread:
	pop	{r0, r1}
	@ r0 = user data
	@ r1 -> thread's main function
	ldr	lr, =exit_thread
	bx	r1
