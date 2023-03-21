//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// iov.h - utilities for managing scatter-gather lists
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

struct _trstack;

typedef struct {

    unsigned capacity;
    unsigned count;
    struct iovec iov[1];

} triov;

triov tr_iov_single(void *buffer, unsigned bufbytes);

unsigned tr_iov_measure(unsigned nbuffers);

void tr_iov_init(unsigned nbuffers, triov *iov);

trstatus tr_iov_alloc(
        unsigned nbuffers,
        tralloctag tag,
        triov **iov);

trstatus tr_iov_stackalloc(
        unsigned nbuffers,
        struct _trstack *stack,
        triov **iov);

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

typedef struct {

    unsigned index;
    unsigned offset;
    unsigned voffset;

} triovpos;

triovpos tr_iov_start(const triov *iov);

triovpos tr_iov_end(const triov *iov);

triovpos tr_iov_advance(const triov *iov, const triovpos *pos, unsigned bytes);

triovpos tr_iov_rewind(const triov *iov, const triovpos *pos, unsigned bytes);
