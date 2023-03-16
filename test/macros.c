/////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>
#include <runtime/macros.h>

static void macro_swap()
{
    int a = 1;
    int b = 2;

    swap(int, a, b);

    TEST_EQUAL(a, 2);
    TEST_EQUAL(b, 1);
}

static void macro_min()
{
    TEST_EQUAL(min(1, 2), 1);
    TEST_EQUAL(min(2, 1), 1);
}

static void macro_max()
{
    TEST_EQUAL(max(2, 1), 2);
    TEST_EQUAL(max(1, 2), 2);
}

static void macro_abs()
{
    TEST_EQUAL(abs(2), 2);
    TEST_EQUAL(abs(-2), 2);
    TEST_EQUAL(abs(0), 0);
}

static void macro_arraysize()
{
    int arr[] = { 1, 2, 3 };

    TEST_EQUAL(arraysize(arr), 3);
}

static void macro_ptr_add()
{
    int arr[] = { 1, 2, 3 };

    TEST_EQUAL(ptr_add(arr, 0), arr);
    TEST_EQUAL(ptr_add(arr, sizeof(int)), arr + 1);
    TEST_EQUAL(ptr_add(arr, 2 * sizeof(int)), arr + 2);
}

static void macro_ptr_sub()
{
    int arr[] = { 1, 2, 3 };

    int *end = arr + 2;
    TEST_EQUAL(ptr_sub(end, 0), arr + 2);
    TEST_EQUAL(ptr_sub(end, sizeof(int)), arr + 1);
    TEST_EQUAL(ptr_sub(end, 2 * sizeof(int)), arr);
}

static void macro_ptr_dist()
{
    int arr[] = { 1, 2, 3 };
    TEST_EQUAL(ptr_dist(arr, arr + 2), 2 * sizeof(int));
}

static void macro_ptr_align()
{
    uintptr_t base;

    for (unsigned i = 0; i < sizeof(base); ++i) {
        TEST_EQUAL(
                ptr_align(ptr_add(&base, i), sizeof(base)),
                (&base) + 1
            );
    }
}

static void macro_offsetof()
{
    typedef struct __attribute__((packed)) {
        int a;
        int b;
        int c;
    } s;

    TEST_EQUAL(offsetof(s, a), 0);
    TEST_EQUAL(offsetof(s, b), sizeof(int));
    TEST_EQUAL(offsetof(s, c), 2 * sizeof(int));
}

static void macro_field_offset()
{
    typedef struct __attribute__((packed)) {
        int a;
        int b;
        int c;
    } s;

    TEST_EQUAL(field_offset(s, a), 0);
    TEST_EQUAL(field_offset(s, b), sizeof(int));
    TEST_EQUAL(field_offset(s, c), 2 * sizeof(int));
}

static void macro_field_size()
{
    typedef struct __attribute__((packed)) {
        int a;
        int b;
        int c;
        long long d;
        char e;
    } s;

    TEST_EQUAL(field_size(s, a), sizeof(int));
    TEST_EQUAL(field_size(s, b), sizeof(int));
    TEST_EQUAL(field_size(s, c), sizeof(int));
    TEST_EQUAL(field_size(s, d), sizeof(long long));
    TEST_EQUAL(field_size(s, e), sizeof(char));
}

static void macro_container_of()
{
    typedef struct __attribute__((packed)) {
        int a;
        int b;
        int c;
        long long d;
        char e;
    } s;

    s var;

    TEST_EQUAL(container_of(&var.a, s, a), &var);
    TEST_EQUAL(container_of(&var.b, s, b), &var);
    TEST_EQUAL(container_of(&var.c, s, c), &var);
    TEST_EQUAL(container_of(&var.d, s, d), &var);
    TEST_EQUAL(container_of(&var.e, s, e), &var);
}

static void macro_to_string()
{
    TEST_TRUE(0 == strcmp(to_string(1 + 2), "1 + 2"));
}

static const test_case macro_cases[] =
{
    TEST_CASE(macro_swap),
    TEST_CASE(macro_min),
    TEST_CASE(macro_max),
    TEST_CASE(macro_abs),
    TEST_CASE(macro_arraysize),
    TEST_CASE(macro_ptr_add),
    TEST_CASE(macro_ptr_sub),
    TEST_CASE(macro_ptr_dist),
    TEST_CASE(macro_ptr_align),
    TEST_CASE(macro_offsetof),
    TEST_CASE(macro_field_offset),
    TEST_CASE(macro_field_size),
    TEST_CASE(macro_container_of),
    TEST_CASE(macro_to_string),
};

TEST_SUITE(macro_tests, macro_cases);
