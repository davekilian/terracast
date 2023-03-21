//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>

#include <runtime/iov.h>
#include <runtime/stack.h>

triov tr_iov_single(void *buffer, unsigned bufbytes)
{
    triov iov;
    iov.capacity = 1;
    iov.count = 1;
    iov.iov[0].iov_base = buffer;
    iov.iov[0].iov_len = bufbytes;

    return iov;
}

unsigned tr_iov_measure(unsigned nbuffers)
{
    tr_assert(nbuffers > 0);
    return sizeof(triov) + (nbuffers - 1) * sizeof(struct iovec);
}

void tr_iov_initialize(triov *iov, unsigned nbuffers)
{
    memset(iov, 0, tr_iov_measure(nbuffers));
    iov->capacity = nbuffers;
}

trstatus tr_iov_alloc(
        unsigned nbuffers,
        tralloctag tag,
        triov **result)
{
    triov *iov = tr_alloc(tr_iov_measure(nbuffers), tag);
    if (iov == NULL) {
        return trstatus_no_mem;
    }

    tr_iov_initialize(iov, nbuffers);
    *result = iov;

    return trstatus_ok;
}

trstatus tr_iov_stackalloc(
        unsigned nbuffers,
        struct _trstack *stack,
        triov **result)
{
    triov *iov = tr_stack_alloc(stack, tr_iov_measure(nbuffers));
    if (iov == NULL) {
        return trstatus_no_mem;
    }

    tr_iov_initialize(iov, nbuffers);
    *result = iov;

    return trstatus_ok;
}

triovpos tr_iov_start(const triov *iov)
{
    tr_assert(iov->count > 0);
    (void)iov;

    triovpos pos = { .index = 0, .offset = 0, .voffset = 0 };
    return pos;
}

triovpos tr_iov_end(const triov *iov)
{
    triovpos pos = { .index = iov->count, .offset = 0, .voffset = 0 };

    for (unsigned i = 0; i < iov->count; ++i) {
        pos.voffset += iov->iov[i].iov_len;
    }

    return pos;
}

triovpos tr_iov_at(const triov *iov, unsigned offset)
{
    unsigned remain = offset;

    for (unsigned i = 0; i < iov->count; ++i) {
        if (remain < iov->iov[i].iov_len) {
            triovpos pos = { .index = i, .offset = remain, .voffset = offset };
            return pos;
        } else {
            remain -= iov->iov[i].iov_len;
        }
    }

    return tr_iov_end(iov);
}

triovpos tr_iov_advance(
        const triov *iov,
        const triovpos *pos,
        unsigned bytes)
{
    triovpos newpos = *pos;

    while (bytes > 0 && newpos.index < iov->count) {

        const struct iovec *item = iov->iov + newpos.index;

        tr_assert(newpos.offset <= item->iov_len);
        unsigned rest = item->iov_len - pos->offset;

        if (bytes < rest) {
            newpos.offset += bytes;
            newpos.voffset += bytes;
            bytes = 0;
        } else {
            newpos.index += 1;
            newpos.offset = 0;
            newpos.voffset += rest;
            bytes -= rest;
        }
    }

    return newpos;
}

triovpos tr_iov_rewind(
        const triov *iov,
        const triovpos *pos,
        unsigned bytes);

unsigned tr_iov_measure_slice(
        const triov *base,
        unsigned offset,
        unsigned size)
{
    tr_assert(size > 0);

    triovpos start = tr_iov_at(base, offset);
    triovpos end = tr_iov_advance(base, &start, size - 1);
    return (end.index - start.index) + 1;
}

void tr_iov_slice(
        const triov *base,
        triov *slice,
        unsigned offset,
        unsigned size)
{
    tr_assert(size > 0);

    // Find the start and end positions within the base list
    triovpos start = tr_iov_at(base, offset);
    triovpos end = tr_iov_advance(base, &start, size - 1);

    // Figure out how many bufers are referenced in this slice
    unsigned length = (end.index - start.index) + 1;
    tr_assert(slice->capacity >= length);

    // Copy the original buffers into the slice
    slice->count = length;
    memcpy(slice->iov, base->iov + start.index, sizeof(struct iovec) * length);

    // Trim the start of the first buffer
    slice->iov[0].iov_base = ptr_add(slice->iov[0].iov_base, start.offset);
    slice->iov[0].iov_len = slice->iov[0].iov_len - start.offset;

    // Trim the end of the last buffer
    tr_assert(end.index < base->count);
    slice->iov[length - 1].iov_len = end.offset;
}

trstatus tr_iov_alloc_slice(
        const triov *base,
        triov *slice,
        unsigned offset,
        unsigned size,
        tralloctag tag);

trstatus tr_iov_stackalloc_slice(
        const triov *base,
        triov *slice,
        unsigned offset,
        unsigned size,
        struct _trstack *stack);
