#include <mini-os/os.h>
#include <mini-os/kernel.h>
#include <xen/xen.h>
#include <xen/memory.h>
#include <xen/hvm/params.h>
#include <arm/arch_mm.h>

/*
 * This structure contains start-of-day info, such as pagetable base pointer,
 * address of the shared_info structure, and things like that.
 * On x86, the hypervisor passes it to us. On ARM, we fill it in ourselves.
 */
union start_info_union start_info_union;

/*
 * Shared page for communicating with the hypervisor.
 * Events flags go here, for example.
 */
shared_info_t *HYPERVISOR_shared_info;

extern char shared_info_page[PAGE_SIZE];

static int hvm_get_parameter(int idx, uint64_t *value)
{
    struct xen_hvm_param xhv;
    int ret;

    xhv.domid = DOMID_SELF;
    xhv.index = idx;
    ret = HYPERVISOR_hvm_op(HVMOP_get_param, &xhv);
    if (ret < 0) {
        BUG();
    }
    *value = xhv.value;
    return ret;
}

static void get_console(void)
{
    uint64_t v = -1;

    hvm_get_parameter(HVM_PARAM_CONSOLE_EVTCHN, &v);
    start_info.console.domU.evtchn = v;

    hvm_get_parameter(HVM_PARAM_CONSOLE_PFN, &v);
    start_info.console.domU.mfn = v;

    printk("Console is on port %d\n", start_info.console.domU.evtchn);
    printk("Console ring is at mfn %x\n", start_info.console.domU.mfn);
}

void get_xenbus(void)
{
    uint64_t value;

    if (hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, &value))
	BUG();

    start_info.store_evtchn = (int)value;

    if(hvm_get_parameter(HVM_PARAM_STORE_PFN, &value))
	BUG();
    start_info.store_mfn = (unsigned long)value;
}

/*
 * INITIAL C ENTRY POINT.
 */
void arch_init(void *dtb_pointer)
{
    struct xen_add_to_physmap xatp;

    memset(&__bss_start, 0, &_end - &__bss_start);

    printk("dtb_pointer : %x\n", dtb_pointer);

    /* Map shared_info page */
    xatp.domid = DOMID_SELF;
    xatp.idx = 0;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.gpfn = virt_to_pfn(shared_info_page);
    if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp) != 0)
        BUG();
    HYPERVISOR_shared_info = (struct shared_info *)shared_info_page;

    /* Fill in start_info */
    get_console();
    get_xenbus();

    start_kernel();
}

void
arch_fini(void)
{

}

void
arch_do_exit(void)
{
}
