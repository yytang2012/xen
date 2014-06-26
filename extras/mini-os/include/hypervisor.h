/******************************************************************************
 * hypervisor.h
 * 
 * Hypervisor handling.
 * 
 *
 * Copyright (c) 2002, K A Fraser
 * Copyright (c) 2005, Grzegorz Milos
 * Updates: Aravindh Puthiyaparambil <aravindh.puthiyaparambil@unisys.com>
 */

#ifndef _HYPERVISOR_H_
#define _HYPERVISOR_H_

#include <mini-os/types.h>
#include <xen/xen.h>
#if defined(__i386__)
#include <hypercall-x86_32.h>
#elif defined(__x86_64__)
#include <hypercall-x86_64.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <hypercall-arm.h>
#else
#error "Unsupported architecture"
#endif
#include <mini-os/traps.h>

/*
 * a placeholder for the start of day information passed up from the hypervisor
 */
union start_info_union
{
    start_info_t start_info;
    char padding[512];
};
extern union start_info_union start_info_union;
#define start_info (start_info_union.start_info)

/*
 * In EVENT_MODE_INTERRUPTS, we respond to interrupts by clearing
 * evtchn_upcall_pending and then calling do_event for each active channel, all
 * within the interrupt handler.
 *
 * In EVENT_MODE_POLLING, we only clear evtchn_upcall_pending. The application
 * should check for pending events when it's ready to handle them (e.g.
 * whenever it's about to block).
 */
#define EVENT_MODE_INTERRUPTS 1
#define EVENT_MODE_POLLING 2

extern int minios_event_handling_mode;

/* hypervisor.c */
void force_evtchn_callback(void);
void do_hypervisor_callback(struct pt_regs *regs);
void mask_evtchn(uint32_t port);
void unmask_evtchn(uint32_t port);
void clear_evtchn(uint32_t port);

extern int in_callback;

#endif /* __HYPERVISOR_H__ */
