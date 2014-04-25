/* 
 ****************************************************************************
 * (C) 2006 - Cambridge University
 ****************************************************************************
 *
 *        File: gnttab.c
 *      Author: Steven Smith (sos22@cam.ac.uk) 
 *     Changes: Grzegorz Milos (gm281@cam.ac.uk)
 *              
 *        Date: July 2006
 * 
 * Environment: Xen Minimal OS
 * Description: Simple grant tables implementation. About as stupid as it's
 *  possible to be and still work.
 *
 ****************************************************************************
 */
#include <mini-os/os.h>
#include <mini-os/mm.h>
#include <mini-os/gnttab.h>
#include <mini-os/semaphore.h>
#include <mini-os/hypervisor.h>
#include <xen/memory.h>
#include <libfdt.h>

extern void *device_tree;

#define NR_RESERVED_ENTRIES 8

/* NR_GRANT_FRAMES must be less than or equal to that configured in Xen */
#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * PAGE_SIZE / sizeof(grant_entry_t))

static grant_entry_t *gnttab_table;
static grant_ref_t gnttab_list[NR_GRANT_ENTRIES];
#ifdef GNT_DEBUG
static char inuse[NR_GRANT_ENTRIES];
#endif
static __DECLARE_SEMAPHORE_GENERIC(gnttab_sem, 0);

static void
put_free_entry(grant_ref_t ref)
{
    unsigned long flags;
    local_irq_save(flags);
#ifdef GNT_DEBUG
    BUG_ON(!inuse[ref]);
    inuse[ref] = 0;
#endif
    gnttab_list[ref] = gnttab_list[0];
    gnttab_list[0]  = ref;
    local_irq_restore(flags);
    up(&gnttab_sem);
}

static grant_ref_t
get_free_entry(void)
{
    unsigned int ref;
    unsigned long flags;
    down(&gnttab_sem);
    local_irq_save(flags);
    ref = gnttab_list[0];
    BUG_ON(ref < NR_RESERVED_ENTRIES || ref >= NR_GRANT_ENTRIES);
    gnttab_list[0] = gnttab_list[ref];
#ifdef GNT_DEBUG
    BUG_ON(inuse[ref]);
    inuse[ref] = 1;
#endif
    local_irq_restore(flags);
    return ref;
}

grant_ref_t
gnttab_grant_access(domid_t domid, unsigned long frame, int readonly)
{
    grant_ref_t ref;

    ref = get_free_entry();
    gnttab_table[ref].frame = frame;
    gnttab_table[ref].domid = domid;
    wmb();
    readonly *= GTF_readonly;
    gnttab_table[ref].flags = GTF_permit_access | readonly;

    return ref;
}

grant_ref_t
gnttab_grant_transfer(domid_t domid, unsigned long pfn)
{
    grant_ref_t ref;

    ref = get_free_entry();
    gnttab_table[ref].frame = pfn;
    gnttab_table[ref].domid = domid;
    wmb();
    gnttab_table[ref].flags = GTF_accept_transfer;

    return ref;
}

int
gnttab_end_access(grant_ref_t ref)
{
    uint16_t flags, nflags;

    BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);

    nflags = gnttab_table[ref].flags;
    do {
        if ((flags = nflags) & (GTF_reading|GTF_writing)) {
            printk("WARNING: g.e. still in use! (%x)\n", flags);
            return 0;
        }
    } while ((nflags = synch_cmpxchg(&gnttab_table[ref].flags, flags, 0)) !=
            flags);

    put_free_entry(ref);
    return 1;
}

unsigned long
gnttab_end_transfer(grant_ref_t ref)
{
    unsigned long frame;
    uint16_t flags;

    BUG_ON(ref >= NR_GRANT_ENTRIES || ref < NR_RESERVED_ENTRIES);

    while (!((flags = gnttab_table[ref].flags) & GTF_transfer_committed)) {
        if (synch_cmpxchg(&gnttab_table[ref].flags, flags, 0) == flags) {
            printk("Release unused transfer grant.\n");
            put_free_entry(ref);
            return 0;
        }
    }

    /* If a transfer is in progress then wait until it is completed. */
    while (!(flags & GTF_transfer_completed)) {
        flags = gnttab_table[ref].flags;
    }

    /* Read the frame number /after/ reading completion status. */
    rmb();
    frame = gnttab_table[ref].frame;

    put_free_entry(ref);

    return frame;
}

grant_ref_t
gnttab_alloc_and_grant(void **map)
{
    unsigned long mfn;
    grant_ref_t gref;

    *map = (void *)alloc_page();
    mfn = virt_to_mfn(*map);
    gref = gnttab_grant_access(0, mfn, 0);
    return gref;
}

static const char * const gnttabop_error_msgs[] = GNTTABOP_error_msgs;

const char *
gnttabop_error(int16_t status)
{
    status = -status;
    if (status < 0 || status >= ARRAY_SIZE(gnttabop_error_msgs))
        return "bad status";
    else
        return gnttabop_error_msgs[status];
}

/* Get Xen's sugggested physical page assignments for the grant table. */
static grant_entry_t *get_gnttab_base(void)
{
    int hypervisor;

    hypervisor = fdt_path_offset(device_tree, "/hypervisor");
    BUG_ON(hypervisor < 0);

    int len = 0;
    const uint64_t *regs = fdt_getprop(device_tree, hypervisor, "reg", &len);
    if (regs == NULL || len != 16) {
            printk("Bad 'reg' property: %p %d\n", regs, len);
            BUG();
    }

    unsigned int gnttab_base = fdt64_to_cpu(regs[0]);

    printk("FDT suggests grant table base %lx\n", gnttab_base);

    return (grant_entry_t *) gnttab_base;
}

void
init_gnttab(void)
{
    struct xen_add_to_physmap xatp;
    struct gnttab_setup_table setup;
    xen_pfn_t frames[NR_GRANT_FRAMES];
    int i, rc;
    gnttab_table = get_gnttab_base();

#ifdef GNT_DEBUG
    memset(inuse, 1, sizeof(inuse));
#endif
    for (i = NR_RESERVED_ENTRIES; i < NR_GRANT_ENTRIES; i++)
        put_free_entry(i);

    for (i = 0; i < NR_GRANT_FRAMES; i++)
    {
        xatp.domid = DOMID_SELF;
        xatp.size = 0;      /* Seems to be unused */
        xatp.space = XENMAPSPACE_grant_table;
        xatp.idx = i;
        xatp.gpfn = (((unsigned long) gnttab_table) >> PAGE_SHIFT) + i;
        rc = HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp);
        BUG_ON(rc != 0);
    }

    setup.dom = DOMID_SELF;
    setup.nr_frames = NR_GRANT_FRAMES;
    set_xen_guest_handle(setup.frame_list, frames);

    setup.status = -1;
    HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
    if (setup.status != 0)
    {
        printk("GNTTABOP_setup_table failed; status = %d\n", setup.status);
        BUG();
    }

    for (i = 0; i < NR_GRANT_FRAMES; i++)
        printk("frames[%d] = %lx\n", i, frames[i]);

    printk("gnttab_table mapped at %p.\n", gnttab_table);
}

void
fini_gnttab(void)
{
    struct gnttab_setup_table setup;

    setup.dom = DOMID_SELF;
    setup.nr_frames = 0;

    HYPERVISOR_grant_table_op(GNTTABOP_setup_table, &setup, 1);
}
