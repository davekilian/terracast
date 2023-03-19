/////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>
#include <runtime/alloc.h>

static void alloc_tagstr()
{
    char str[5];
    tr_alloctag_tostr('abcd', str);
    TEST_TRUE(0 == strcmp(str, "abcd"));
}

static void alloc_basic()
{
    char *buffer = tr_alloc(128, 'test');
    TEST_NOT_NULL(buffer);

    for (int i = 0; i < 128; ++i) {
        buffer[i] = 0;
    }

    tr_free(buffer);
}

static void alloc_stats()
{
    trallocstat stat = tr_alloc_stat('test');
    TEST_EQUAL(stat.tag, 'test');
    TEST_EQUAL(stat.nalloc, 0);
    TEST_EQUAL(stat.nbytes, 0);

    char *buffer = tr_alloc(128, 'test');

    stat = tr_alloc_stat('test');
    TEST_EQUAL(stat.tag, 'test');
    TEST_EQUAL(stat.nalloc, 1);
    TEST_EQUAL(stat.nbytes, 128);

    tr_free(buffer);

    stat = tr_alloc_stat('test');
    TEST_EQUAL(stat.tag, 'test');
    TEST_EQUAL(stat.nalloc, 0);
    TEST_EQUAL(stat.nbytes, 0);

    trallocstat *stats = tr_alloc(4 * sizeof(trallocstat), 'test');
    TEST_EQUAL(1, tr_alloc_stats(stats, 4));
    TEST_EQUAL(stats[0].tag, 'test');
    TEST_EQUAL(stats[0].nalloc, 1);
    TEST_EQUAL(stats[0].nbytes, 4 * sizeof(trallocstat));
}

static const test_case alloc_cases[] =
{
    TEST_CASE(alloc_tagstr),
    TEST_CASE(alloc_basic),
    TEST_CASE(alloc_stats),
};

TEST_SUITE(alloc_tests, alloc_cases);
