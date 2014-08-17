/******************************************************************************
 * profile.c
 *
 * Record profiling information.
 *
 * Copyright (c) 2014, Thomas Leonard
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <mini-os/profile.h>
#include <mini-os/xmalloc.h>

struct trace_entry *trace_buffer;
struct trace_entry *trace_buffer_end;

void profile_init(struct trace_entry *buffer, size_t size)
{
    trace_buffer = buffer;
    if (buffer)
        trace_buffer_end = buffer + size;
    else
        trace_buffer_end = NULL;
}

size_t profile_get_count(void)
{
    return (trace_buffer_end - trace_buffer) / sizeof(trace_entry);
}
