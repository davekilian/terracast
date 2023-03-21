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

triovpos tr_iov_advance(
        const triov *iov,
        const triovpos *pos,
        unsigned bytes);

triovpos tr_iov_rewind(
        const triov *iov,
        const triovpos *pos,
        unsigned bytes);

unsigned tr_iov_measure_slice(
        const triov *base,
        unsigned offset,
        unsigned size);

void tr_iov_slice(
        const triov *base,
        triov *slice,
        unsigned offset,
        unsigned size);

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
