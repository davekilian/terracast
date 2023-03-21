//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.  
//
// stack.h - x86-like stack for async operations
//
// The trstack type allows async requests to dynamically allocate local memory
// for the request. This memory cannot be allocated on the calling thread
// stack since the calling thread may unwind due to async I/O (to be resumed
// from another thread on I/O completion), and using the heap is undesirable
// since the memory is locally owned to a single request. Instead, you can
// preallocate a trstack associated with the request, and do any dynamic
// allocations for that request on the request's local trstack. The trstack
// is thus affinitized to the request, not the calling thread.
//
// Because trstack is intended for use with async request systems, there is
// no internal synchronization or locking. You may allocate memory from only
// one thread at a time. In practice, this is usually easy, because usually
// an async request is being processed by one one thread at a time.
//
// trstack supports 'stack frames', which group a set of allocations. If you
// call enter(), then make some allocations, then call leave(), upon the call
// to leave(), all allocations made since you called enter() are freed, but
// older allocations remain active. This is analogous to how procedure calls
// work on a real x86 stack. You can use frames to manage your memory in the
// middle of an async operation, or you may opt to simply alloc() whenever
// needed and then clear() all allocations at the end of the request.
//
// If you call alloc() and there's too little space left in the stack to
// accommodate your allocation, tr_stack_alloc will automatically heap-
// allocate another stack segment, of the same size as your previous stack
// segment, and allocate all memory starting from there going forward. These
// segments are automatically cleaned up when leave() or clear() leaves a
// segment unused. As long segments are allocated infrequently, this is
// unlikely to cause excessive heap activity, but if you want to disallow
// this behavior, set flags.grows to 0 after creating your trstack.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <runtime/list.h>

// Storage for a single segment of a growable stack
typedef struct _trstackseg {

    struct _trstackseg *next;   // Next-newest segment of the stack
    void *startptr;             // Start address (low, inlusive)
    void *endptr;               // End address (high, exclusive)
    char align[8];              // For 16-byte alignment

} trstackseg;

static_assert(sizeof(trstackseg) % 16 == 0);

// A reusable push-allocator suitable for use with async tasks
typedef struct {

    trstackseg *segments;   // Storage segments, newest to oldest
    void *stackptr;         // Where to make the next allocation
    void *frameptr;         // Points to the base of the previous frame
    tralloctag tag;         // Caller-owned tag to pass down on heap alloc
    struct {
        unsigned grows : 1; // Whether new segments can be allocated
    } flags;

} trstack;

// Initializes a stack with a single heap-allocated segments with the given
// number of bytes of storage.
//
trstatus tr_stack_initialize(trstack *stack, unsigned bytes, tralloctag tag);

// Frees all underlying segments for the given stack
void tr_stack_cleanup(trstack *stack);

// Allocates data within the active stack segment, growing if necesssary
void *tr_stack_alloc(trstack *stack, unsigned bytes);

// Pushes a new stack frame onto the stack
//
// Balancing this call with a call to tr_stack_leave() frees all alloc()s
// that occurred since the matching call to this routine. Note that frames
// are recursive: you can enter and leave a stack frame from within another
// active frame. Think of frames as procedure calls on an x86 stack.
//
// You do not need to call leave() if you plan to clear() the stack instead.
// However, it may be advantageous to isolate large, short-lived allocations
// to a stack frame and clean them up mid-request so you don't overrun your
// stack and force it to grow unnecessariy.
//
trstatus tr_stack_enter(trstack *stack);

// Leaves the most recently entered stack frame that is still active. All
// allocations made since that stack frame was entered will be invalidated.
//
// You do not need to call this if you plan to clear() the stack instead.
//
void tr_stack_leave(trstack *stack);

// Returns the stack to its initial state:
//
// - All segments which were automatically allocated are freed
// - All space in the original stack segment is freed
// - All frames are cleaned up
//
// This routine is appropriate for use before recycling a stack which may or
// may not still have active allocations.
//
void tr_stack_clear(trstack *stack);
