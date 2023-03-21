//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <runtime/stack.h>

static trstackseg *tr_stack_alloc_seg(unsigned bytes, tralloctag tag)
{
    trstackseg *seg = tr_alloc(sizeof(trstackseg) + bytes, tag);
    if (seg == NULL) {
        return NULL;
    }

    seg->next = NULL;
    seg->startptr = seg + 1;
    seg->endptr = ptr_add(seg->startptr, bytes);
    return seg;
}

trstatus tr_stack_initialize(trstack *stack, unsigned bytes, tralloctag tag)
{
    trstackseg *seg = tr_stack_alloc_seg(bytes, tag);
    if (seg == NULL) {
        return trstatus_no_mem;
    }

    stack->segments = seg;
    stack->stackptr = seg->startptr;
    stack->frameptr = NULL;
    stack->tag = tag;
    stack->flags.grows = 1;

    return trstatus_ok;
}

void tr_stack_cleanup(trstack *stack)
{
    while (stack->segments != NULL) {
        trstackseg *freeseg = stack->segments;
        stack->segments = stack->segments->next;
        tr_free(freeseg);
    }

    stack->stackptr = NULL;
    stack->frameptr = NULL;
}

void *tr_stack_alloc(trstack *stack, unsigned bytes)
{
    trstackseg *seg = stack->segments;

    unsigned limit = ptr_dist(seg->startptr, seg->endptr);
    if (bytes > limit) {
        tr_assert(0 && "alloc size too big for a stack segment");
        return NULL;
    }

    if (ptr_add(stack->stackptr, bytes) > seg->endptr) {

        if (!stack->flags.grows) {
            tr_assert(0 && "trstack memory exhausted");
            return NULL;
        }

        trstackseg *newseg = tr_stack_alloc_seg(limit, stack->tag);
        if (newseg == NULL) {
            return NULL;
        }

        stack->segments = newseg;
        newseg->next = seg;
        seg = newseg;

        stack->stackptr = seg->startptr;
    }

    tr_assert(stack->stackptr >= seg->startptr);
    tr_assert(stack->stackptr <= seg->endptr);

    void *dataptr = stack->stackptr;
    stack->stackptr = ptr_add(stack->stackptr, bytes);

    tr_assert(stack->stackptr >= seg->startptr);
    tr_assert(stack->stackptr <= seg->endptr);

    return dataptr;
}

trstatus tr_stack_enter(trstack *stack)
{
    void **save = tr_stack_alloc(stack, sizeof(void *));
    if (save == NULL) {
        return trstatus_no_mem;
    }

    *save = stack->frameptr;
    stack->frameptr = save;

    return trstatus_ok;
}

void tr_stack_leave(trstack *stack)
{
    stack->stackptr = stack->frameptr;
    stack->frameptr = *(void **)stack->frameptr;

    trstackseg *newhead = stack->segments;
    while (stack->stackptr < newhead->startptr ||
           stack->stackptr > newhead->endptr) {

        tr_assert(newhead->next != NULL);
        newhead = newhead->next;
    }

    if (stack->stackptr == newhead->startptr &&
        newhead->next != NULL) {

        newhead = newhead->next;
    }

    while (stack->segments != newhead) {
        trstackseg *freeseg = stack->segments;
        stack->segments = stack->segments->next;
        tr_free(freeseg);
    }
}

void tr_stack_clear(trstack *stack)
{
    while (stack->segments->next != NULL) {
        trstackseg *freeseg = stack->segments;
        stack->segments = stack->segments->next;
        tr_free(freeseg);
    }

    trstackseg *head = stack->segments;
    stack->stackptr = head->startptr;
    stack->frameptr = NULL;
}
