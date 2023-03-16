/////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>
#include <runtime/status.h>

static void status_build()
{
    trstatus s = tr_status(0, trstatus_errno, 123);
    TEST_EQUAL(s & trstatus_fail_mask, trstatus_fail_mask);
    TEST_EQUAL(s & trstatus_origin_mask, trstatus_errno);
    TEST_EQUAL(s & trstatus_code_mask, 123);

    s = tr_status(1, trstatus_http, 321);
    TEST_EQUAL(s & trstatus_fail_mask, 0);
    TEST_EQUAL(s & trstatus_origin_mask, trstatus_http);
    TEST_EQUAL(s & trstatus_code_mask, 321);
}

static void status_ok()
{
    TEST_TRUE(tr_ok(trstatus_ok));
    TEST_FALSE(tr_ok(trstatus_fail));

    TEST_TRUE(tr_failed(trstatus_fail));
    TEST_FALSE(tr_failed(trstatus_ok));
}

static const test_case status_cases[] =
{
    TEST_CASE(status_build),
    TEST_CASE(status_ok),
};

TEST_SUITE(status_tests, status_cases);
