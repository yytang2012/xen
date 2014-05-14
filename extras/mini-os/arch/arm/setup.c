#include <mini-os/os.h>
#include <mini-os/kernel.h>
#include <xen/xen.h>
#include <xen/memory.h>
#include <xen/hvm/params.h>
#include <arch_mm.h>
#include <libfdt.h>

/*
 * This structure contains start-of-day info, such as pagetable base pointer,
 * address of the shared_info structure, and things like that.
 * On x86, the hypervisor passes it to us. On ARM, we fill it in ourselves.
 */
union start_info_union start_info_union;

extern void *fault_reset;
extern void *fault_undefined_instruction;
extern void *fault_svc;
extern void *fault_prefetch_call;
extern void *fault_prefetch_abort;
extern void *fault_data_abort;

/*
 * Shared page for communicating with the hypervisor.
 * Events flags go here, for example.
 */
shared_info_t *HYPERVISOR_shared_info;

extern char shared_info_page[PAGE_SIZE];

void *device_tree;

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

    printk("Checking DTB at %x...\n", dtb_pointer);

    int r;
    if ((r = fdt_check_header(dtb_pointer))) {
        printk("Invalid DTB from Xen: %s\n", fdt_strerror(r));
        BUG();
    }
    device_tree = dtb_pointer;

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

    /* Map shared_info page */
    xatp.domid = DOMID_SELF;
    xatp.idx = 0;
    xatp.space = XENMAPSPACE_shared_info;
    xatp.gpfn = virt_to_pfn(shared_info_page);
    if (HYPERVISOR_memory_op(XENMEM_add_to_physmap, &xatp) != 0)
        BUG();
    HYPERVISOR_shared_info = (struct shared_info *)shared_info_page;

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

void dump_registers(int *saved_registers) {
    char *fault_name;
    void *fault_handler;
    int i;
    printk("Fault!\n");
    for (i = 0; i < 16; i++) {
        printk("r%d = %x\n", i, saved_registers[i]);
    }
    printk("CPSR = %x\n", saved_registers[16]);
    fault_handler = (void *) saved_registers[17];
    if (fault_handler == &fault_reset)
	fault_name = "reset";
    else if (fault_handler == &fault_undefined_instruction)
	fault_name = "undefined_instruction";
    else if (fault_handler == &fault_svc)
	fault_name = "svc";
    else if (fault_handler == &fault_prefetch_call)
	fault_name = "prefetch_call";
    else if (fault_handler == &fault_prefetch_abort)
	fault_name = "prefetch_abort";
    else if (fault_handler == &fault_data_abort)
	fault_name = "data_abort";
    else
	fault_name = "unknown fault type!";

    printk("Fault handler at %x called (%s)\n", fault_handler, fault_name);
}
