#ifndef _ARCH_MM_H_
#define _ARCH_MM_H_

extern char _text, _etext, _erodata, _edata, _end, __bss_start;
extern int stack[];
extern int physical_address_offset;	/* Add this to a virtual address to get the physical address (wraps) */

#define PAGE_SHIFT        12
#define PAGE_SIZE        (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define L1_PAGETABLE_SHIFT      12

#define to_phys(x)                 ((unsigned long)(x)+physical_address_offset)
#define to_virt(x)                 ((void *)((unsigned long)(x)-physical_address_offset))

#define PFN_UP(x)    (((x) + PAGE_SIZE-1) >> L1_PAGETABLE_SHIFT)
#define PFN_DOWN(x)    ((x) >> L1_PAGETABLE_SHIFT)
#define PFN_PHYS(x)    ((uint64_t)(x) << L1_PAGETABLE_SHIFT)
#define PHYS_PFN(x)    ((x) >> L1_PAGETABLE_SHIFT)

#define virt_to_pfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define mfn_to_virt(_mfn)          (to_virt(PFN_PHYS(_mfn)))
#define pfn_to_virt(_pfn)          (to_virt(PFN_PHYS(_pfn)))

#define mfn_to_pfn(x) (x)
#define pfn_to_mfn(x) (x)

#define virtual_to_mfn(_virt)	   virt_to_mfn(_virt)

// FIXME
#define map_frames(f, n) (NULL)

#endif
