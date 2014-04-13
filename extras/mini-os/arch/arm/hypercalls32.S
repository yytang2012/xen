#define __HYPERVISOR_set_trap_table        0
#define __HYPERVISOR_mmu_update            1
#define __HYPERVISOR_set_gdt               2
#define __HYPERVISOR_stack_switch          3
#define __HYPERVISOR_set_callbacks         4
#define __HYPERVISOR_fpu_taskswitch        5
#define __HYPERVISOR_sched_op_compat       6 /* compat since 0x00030101 */
#define __HYPERVISOR_platform_op           7
#define __HYPERVISOR_set_debugreg          8
#define __HYPERVISOR_get_debugreg          9
#define __HYPERVISOR_update_descriptor    10
#define __HYPERVISOR_memory_op            12
#define __HYPERVISOR_multicall            13
#define __HYPERVISOR_update_va_mapping    14
#define __HYPERVISOR_set_timer_op         15
#define __HYPERVISOR_event_channel_op_compat 16 /* compat since 0x00030202 */
#define __HYPERVISOR_xen_version          17
#define __HYPERVISOR_console_io           18
#define __HYPERVISOR_physdev_op_compat    19 /* compat since 0x00030202 */
#define __HYPERVISOR_grant_table_op       20
#define __HYPERVISOR_vm_assist            21
#define __HYPERVISOR_update_va_mapping_otherdomain 22
#define __HYPERVISOR_iret                 23 /* x86 only */
#define __HYPERVISOR_vcpu_op              24
#define __HYPERVISOR_set_segment_base     25 /* x86/64 only */
#define __HYPERVISOR_mmuext_op            26
#define __HYPERVISOR_xsm_op               27
#define __HYPERVISOR_nmi_op               28
#define __HYPERVISOR_sched_op             29
#define __HYPERVISOR_callback_op          30
#define __HYPERVISOR_xenoprof_op          31
#define __HYPERVISOR_event_channel_op     32
#define __HYPERVISOR_physdev_op           33
#define __HYPERVISOR_hvm_op               34
#define __HYPERVISOR_sysctl               35
#define __HYPERVISOR_domctl               36
#define __HYPERVISOR_kexec_op             37
#define __HYPERVISOR_tmem_op              38
#define __HYPERVISOR_xc_reserved_op       39 /* reserved for XenClient */



#define __HVC(imm16) .long ((0xE1400070 | (((imm16) & 0xFFF0) << 4) | ((imm16) & 0x000F)) & 0xFFFFFFFF)

#define XEN_IMM 0xEA1

#define HYPERCALL_SIMPLE(hypercall)		\
.globl HYPERVISOR_##hypercall;			\
.align 4,0x90;					\
HYPERVISOR_##hypercall:				\
        mov r12, #__HYPERVISOR_##hypercall;	\
        __HVC(XEN_IMM);				\
        mov pc, lr;

#define _hypercall0 HYPERCALL_SIMPLE
#define _hypercall1 HYPERCALL_SIMPLE
#define _hypercall2 HYPERCALL_SIMPLE
#define _hypercall3 HYPERCALL_SIMPLE
#define _hypercall4 HYPERCALL_SIMPLE
/* Note: if we ever need _hypercall5, we can't define it this way, as AAPCS only lets us clobber r0-r3 */

_hypercall1(set_trap_table);
_hypercall4(mmu_update);
_hypercall4(mmuext_op);
_hypercall2(set_gdt);
_hypercall2(stack_switch);
_hypercall3(set_callbacks);
_hypercall1(fpu_taskswitch);
_hypercall2(sched_op);
_hypercall1(set_timer_op);
_hypercall2(set_debugreg);
_hypercall1(get_debugreg);
_hypercall2(update_descriptor);
_hypercall2(memory_op);
_hypercall2(multicall);
_hypercall3(update_va_mapping);
_hypercall2(event_channel_op);
_hypercall2(xen_version);
_hypercall3(console_io);
_hypercall1(physdev_op);
_hypercall3(grant_table_op);
_hypercall4(update_va_mapping_otherdomain);
_hypercall2(vm_assist);
_hypercall3(vcpu_op);
_hypercall2(set_segment_base);
_hypercall2(nmi_op);
_hypercall1(sysctl);
_hypercall1(domctl);
_hypercall2(hvm_op);

.align 4

.globl dump_regs
dump_regs:
	@ Write to console
	mov	r0, #0		@ write
	mov	r1, #4 @ buffer_end - buffer
	ldr	r2, =buffer
        mov	r12, #__HYPERVISOR_console_io
        __HVC(XEN_IMM)

	ldr	r2, =buffer2
	add	r1, r2, #(buffer_end2 - buffer2 - 2)
	mov	r0, #'+'
1:
	strb	r0, [r2], #1
	cmp	r2, r1
	bne	1b

	mov	r0, #0		@ write
	mov	r1, #buffer_end2 - buffer2
	ldr	r2, =buffer2
	mov	r3, #'!'
	strb 	r3,[r2,#1]
	strb 	r3,[r2,#10]
        mov	r12, #__HYPERVISOR_console_io
	dsb
	dmb
	isb
        __HVC(XEN_IMM)

	mov	pc, r14


	ldr	r12, =dump
	stmia	r12, {r0-r15}
	mov	r4, #0		@ r4 = reg number
	ldr	r5, =buffer	@ r5 -> buffer
	mov	r6, r12		@ r6 -> saved registers
1:
	ldr	r0, [r6, r4, lsl#2]

	ldr	r5, =buffer	@ r5 -> buffer
	add	r2, r4, #49
	strb	r2,[r5, #1]

	dmb

	cmp r4,#3
	blt skip

	@ Write to console
	mov	r0, #0		@ write
	mov	r1, #buffer_end - buffer
	mov	r2, r5
        mov	r12, #__HYPERVISOR_console_io
        __HVC(XEN_IMM)
skip:

	add	r4, r4, #1
	cmp	r4, #16
	blt	1b

	ldmia	r6, {r0-r13, r15}

.pushsection .data
.align	2
dump:
	.fill (16), 4, 0x0
.align	6
buffer:
	.string "012\n456789abcdefB123456789abcdefC123456789abcdefD123456789a" @ 60 bytes
buffer_end:
buffer2:
	.string "..........................\n"
buffer_end2:
.popsection
