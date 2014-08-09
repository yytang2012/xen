#include <stdint.h>
#include <stddef.h>

struct trace_entry {
    uint64_t time;
    uint32_t callee;
    uint32_t caller;
};

/* Start writing profiling events to the given buffer.
 * Profiling stops when the buffer is full.
 * Pass a NULL buffer to stop tracing. */
void profile_init(struct trace_entry *, size_t);

/* Return the number of entries used so far. */
size_t profile_get_count(void);
