/////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>
#include <runtime/stack.h>

static void stack_create()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(ptr_dist(head->startptr, head->endptr), 16);
    TEST_EQUAL(stack.stackptr, head->startptr);
    TEST_NULL(stack.frameptr);
    TEST_EQUAL(stack.tag, 'test');

    tr_stack_cleanup(&stack);
}

static void stack_alloc()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);
    int *c = tr_stack_alloc(&stack, 4);

    TEST_EQUAL(a + 1, b);
    TEST_EQUAL(b + 1, c);
    TEST_EQUAL(c + 1, stack.stackptr);

    tr_stack_cleanup(&stack);
}

static void stack_clear()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);
    int *c = tr_stack_alloc(&stack, 4);
    TEST_EQUAL(a + 1, b);
    TEST_EQUAL(b + 1, c);
    TEST_EQUAL(c + 1, stack.stackptr);

    tr_stack_clear(&stack);

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(stack.stackptr, head->startptr);

    tr_stack_cleanup(&stack);
}

static void stack_grow()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);
    int *c = tr_stack_alloc(&stack, 4);
    int *d = tr_stack_alloc(&stack, 4);
    int *e = tr_stack_alloc(&stack, 4);

    TEST_EQUAL(a + 1, b);
    TEST_EQUAL(b + 1, c);
    TEST_EQUAL(c + 1, d);
    TEST_EQUAL(d + 1, stack.segments->next->endptr);
    TEST_EQUAL(e, stack.segments->startptr);

    tr_stack_clear(&stack);

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(stack.stackptr, head->startptr);

    tr_stack_cleanup(&stack);
}

static void stack_grow_multi()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *ptrs[16];
    for (int i = 0; i < arraysize(ptrs); ++i) {
        ptrs[i] = tr_stack_alloc(&stack, 4);
    }

    TEST_NOT_NULL(stack.segments);
    TEST_NOT_NULL(stack.segments->next);
    TEST_NOT_NULL(stack.segments->next->next);
    TEST_NOT_NULL(stack.segments->next->next->next);
    TEST_NULL(stack.segments->next->next->next->next);

    for (int i = 0; i < arraysize(ptrs); ++i) {
        *(ptrs[i]) = i;
    }

    for (int i = 0; i < arraysize(ptrs); ++i) {
        TEST_EQUAL(i, *(ptrs[i]));
    }

    tr_stack_clear(&stack);

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(stack.stackptr, head->startptr);

    tr_stack_cleanup(&stack);
}

static void stack_frames()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);

    TEST_SUCCESS(tr_stack_enter(&stack));

    int *c = tr_stack_alloc(&stack, 4);
    int *d = tr_stack_alloc(&stack, 4);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_GREATER_THAN(stack.stackptr, (void *)c);
    TEST_GREATER_THAN(stack.stackptr, (void *)d);

    tr_stack_leave(&stack);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_LESS_EQUAL(stack.stackptr, (void *)c);
    TEST_LESS_EQUAL(stack.stackptr, (void *)d);

    tr_stack_cleanup(&stack);
}

static void stack_grow_frames()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);

    TEST_SUCCESS(tr_stack_enter(&stack));

    int *c = tr_stack_alloc(&stack, 4);
    int *d = tr_stack_alloc(&stack, 4);
    int *e = tr_stack_alloc(&stack, 4);

    TEST_NOT_NULL(stack.segments->next);
    TEST_NULL(stack.segments->next->next);

    tr_stack_leave(&stack);

    TEST_NULL(stack.segments->next);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_LESS_EQUAL(stack.stackptr, (void *)c);
    TEST_LESS_EQUAL(stack.stackptr, (void *)d);
    (void)e;

    tr_stack_cleanup(&stack);
}

static void stack_nest_frames()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 64, 'test'));

    int *a = tr_stack_alloc(&stack, 4);
    int *b = tr_stack_alloc(&stack, 4);

    TEST_SUCCESS(tr_stack_enter(&stack));

    int *c = tr_stack_alloc(&stack, 4);
    int *d = tr_stack_alloc(&stack, 4);

    TEST_SUCCESS(tr_stack_enter(&stack));

    int *e = tr_stack_alloc(&stack, 4);
    int *f = tr_stack_alloc(&stack, 4);

    TEST_SUCCESS(tr_stack_enter(&stack));

    int *g = tr_stack_alloc(&stack, 4);
    int *h = tr_stack_alloc(&stack, 4);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_GREATER_THAN(stack.stackptr, (void *)c);
    TEST_GREATER_THAN(stack.stackptr, (void *)d);
    TEST_GREATER_THAN(stack.stackptr, (void *)e);
    TEST_GREATER_THAN(stack.stackptr, (void *)f);
    TEST_GREATER_THAN(stack.stackptr, (void *)g);
    TEST_GREATER_THAN(stack.stackptr, (void *)h);

    tr_stack_leave(&stack);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_GREATER_THAN(stack.stackptr, (void *)c);
    TEST_GREATER_THAN(stack.stackptr, (void *)d);
    TEST_GREATER_THAN(stack.stackptr, (void *)e);
    TEST_GREATER_THAN(stack.stackptr, (void *)f);
    TEST_LESS_EQUAL(stack.stackptr, (void *)g);
    TEST_LESS_EQUAL(stack.stackptr, (void *)h);

    tr_stack_leave(&stack);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_GREATER_THAN(stack.stackptr, (void *)c);
    TEST_GREATER_THAN(stack.stackptr, (void *)d);
    TEST_LESS_EQUAL(stack.stackptr, (void *)e);
    TEST_LESS_EQUAL(stack.stackptr, (void *)f);
    TEST_LESS_EQUAL(stack.stackptr, (void *)g);
    TEST_LESS_EQUAL(stack.stackptr, (void *)h);

    tr_stack_leave(&stack);

    TEST_GREATER_THAN(stack.stackptr, (void *)a);
    TEST_GREATER_THAN(stack.stackptr, (void *)b);
    TEST_LESS_EQUAL(stack.stackptr, (void *)c);
    TEST_LESS_EQUAL(stack.stackptr, (void *)d);
    TEST_LESS_EQUAL(stack.stackptr, (void *)e);
    TEST_LESS_EQUAL(stack.stackptr, (void *)f);
    TEST_LESS_EQUAL(stack.stackptr, (void *)g);
    TEST_LESS_EQUAL(stack.stackptr, (void *)h);

    tr_stack_cleanup(&stack);
}

static void stack_grow_nested_frames()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *ptrs[16];
    for (int i = 0; i < arraysize(ptrs); ++i) {
        tr_stack_enter(&stack);
        ptrs[i] = tr_stack_alloc(&stack, 4);
    }

    TEST_NOT_NULL(stack.segments);
    TEST_NOT_NULL(stack.segments->next);
    TEST_NOT_NULL(stack.segments->next->next);
    TEST_NOT_NULL(stack.segments->next->next->next);

    for (int i = 0; i < arraysize(ptrs); ++i) {
        *(ptrs[i]) = i;
    }

    for (int i = 0; i < arraysize(ptrs); ++i) {
        TEST_EQUAL(i, *(ptrs[i]));
    }

    for (int i = 0; i < arraysize(ptrs); ++i) {
        tr_stack_leave(&stack);
    }

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(stack.stackptr, head->startptr);

    tr_stack_cleanup(&stack);
}

static void stack_clear_complex()
{
    trstack stack;
    TEST_SUCCESS(tr_stack_initialize(&stack, 16, 'test'));

    int *ptrs[16];
    for (int i = 0; i < arraysize(ptrs); ++i) {
        tr_stack_enter(&stack);
        ptrs[i] = tr_stack_alloc(&stack, 4);
    }

    TEST_NOT_NULL(stack.segments);
    TEST_NOT_NULL(stack.segments->next);
    TEST_NOT_NULL(stack.segments->next->next);
    TEST_NOT_NULL(stack.segments->next->next->next);

    for (int i = 0; i < arraysize(ptrs); ++i) {
        *(ptrs[i]) = i;
    }

    for (int i = 0; i < arraysize(ptrs); ++i) {
        TEST_EQUAL(i, *(ptrs[i]));
    }

    tr_stack_clear(&stack);

    trstackseg *head = stack.segments;
    TEST_NOT_NULL(head);
    TEST_NULL(head->next);
    TEST_EQUAL(stack.stackptr, head->startptr);

    tr_stack_cleanup(&stack);
}

static const test_case stack_cases[] =
{
    TEST_CASE(stack_create),
    TEST_CASE(stack_alloc),
    TEST_CASE(stack_clear),
    TEST_CASE(stack_grow),
    TEST_CASE(stack_grow_multi),
    TEST_CASE(stack_frames),
    TEST_CASE(stack_grow_frames),
    TEST_CASE(stack_nest_frames),
    TEST_CASE(stack_grow_nested_frames),
    TEST_CASE(stack_clear_complex),
};

TEST_SUITE(stack_tests, stack_cases);
