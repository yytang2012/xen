#ifndef _OS_H_
#define _OS_H_

#ifndef __ASSEMBLY__

#include <mini-os/hypervisor.h>
#include <mini-os/types.h>
#include <xen/xen.h>

void arch_fini(void);

#define BUG() while(1){}

#define smp_processor_id() 0

#define barrier() __asm__ __volatile__("": : :"memory")

extern shared_info_t *HYPERVISOR_shared_info;

// disable interrupts
static inline __cli(void) {
    int x;
    __asm__ __volatile__("mrs %0, cpsr;cpsid i":"=r"(x)::"memory");
}

// enable interrupts
static inline __sti(void) {
    int x;
    __asm__ __volatile__("mrs %0, cpsr\n"
                        "bic %0, %0, #0x80\n"
                        "msr cpsr_c, %0"
                        :"=r"(x)::"memory");
}

static inline int irqs_disabled() {
    int x;
    __asm__ __volatile__("mrs %0, cpsr\n":"=r"(x)::"memory");
    return (x & 0x80);
}

#define local_irq_save(x) { \
    __asm__ __volatile__("mrs %0, cpsr;cpsid i; and %0, %0, #0x80":"=r"(x)::"memory");    \
}

#define local_irq_restore(x) {    \
    __asm__ __volatile__("msr cpsr_c, %0"::"r"(x):"memory");    \
}

#define local_save_flags(x)    { \
    __asm__ __volatile__("mrs %0, cpsr; and %0, %0, 0x80":"=r"(x)::"memory");    \
}

#define local_irq_disable()    __cli()
#define local_irq_enable() __sti()

#if defined(__arm__)
#define mb() __asm__("dmb");
#define rmb() __asm__("dmb");
#define wmb() __asm__("dmb");
#elif defined(__aarch64__)
#define mb()
#define rmb()
#define wmb()
#else
#error undefined architecture
#endif

#define unlikely(x)  __builtin_expect((x),0)
#define likely(x)  __builtin_expect((x),1)

/************************** arm *******************************/
#ifdef __INSIDE_MINIOS__
#if defined (__arm__)
#define xchg(ptr,v) __atomic_exchange_n(ptr, v, __ATOMIC_SEQ_CST)

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile void * addr)
{
    uint8_t *byte = ((uint8_t *)addr) + (nr >> 3);
    uint8_t bit = 1 << (nr & 7);
    uint8_t orig;

    orig = __atomic_fetch_and(byte, ~bit, __ATOMIC_SEQ_CST);

    return (orig & bit) != 0;
}

static __inline__ int test_and_set_bit(int nr, volatile void *base)
{
    uint8_t *byte = ((uint8_t *)base) + (nr >> 3);
    uint8_t bit = 1 << (nr & 7);
    uint8_t orig;

    orig = __atomic_fetch_or(byte, bit, __ATOMIC_SEQ_CST);

    return (orig & bit) != 0;
}

static __inline__ int test_bit(int nr, const volatile unsigned long *addr)
{
    const uint8_t *ptr = (const uint8_t *) addr;
    return ((1 << (nr & 7)) & (ptr[nr >> 3])) != 0;
}

/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void set_bit(int nr, volatile unsigned long *addr)
{
    test_and_set_bit(nr, addr);
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void clear_bit(int nr, volatile unsigned long *addr)
{
    test_and_clear_bit(nr, addr);
}

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __inline__ unsigned long __ffs(unsigned long word)
{
    int clz;

    /* xxxxx10000 = word
     * xxxxx01111 = word - 1
     * 0000011111 = word ^ (word - 1)
     *      4     = 31 - clz(word ^ (word - 1))
     */

    __asm__ (
        "sub r0, %[word], #1\n"
        "eor r0, r0, %[word]\n"
        "clz %[clz], r0\n":
        /* Outputs: */
        [clz] "=r"(clz):
        /* Inputs: */
        [word] "r"(word):
        /* Clobbers: */
        "r0");

    return 31 - clz;
}

#else /* ifdef __arm__ */
#error "Unsupported architecture"
#endif
#endif /* ifdef __INSIDE_MINIOS */

/********************* common arm32 and arm64  ****************************/

/* If *ptr == old, then store new there (and return new).
 * Otherwise, return the old value.
 * Atomic. */
#define synch_cmpxchg(ptr, old, new) \
({ __typeof__(*ptr) stored = old; \
   __atomic_compare_exchange_n(ptr, &stored, new, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST) ? new : old; \
})

#define synch_set_bit(n, addr) set_bit(n, addr)
#define synch_clear_bit(n, addr) clear_bit(n, addr)
#define synch_test_and_set_bit(n, addr) test_and_set_bit(n, addr)
#define synch_test_and_clear_bit(n, addr) test_and_clear_bit(n, addr)
#define synch_test_bit(nr, addr) test_bit(nr, addr)

#endif /* not assembly */

#endif
