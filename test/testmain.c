//////////////////////////////////////////////////////////////////////////////
//
// Teracast
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>

extern test_suite macro_tests;
extern test_suite status_tests;

static const test_suite *test_suites[] =
{
    &macro_tests,
    &status_tests,
};

static const int nsuites = arraysize(test_suites);

int main(int argc, const char *argv[])
{
    int total = 0;
    for (int i = 0; i < nsuites; ++i) {
        total += test_suites[i]->ncases;
    }

    int index = 0;
    for (int i = 0; i < nsuites; ++i) {

        const test_suite *suite = test_suites[i];

        for (int j = 0; j < suite->ncases; ++j) {

            const test_case *test = suite->cases + j;

            printf("(%u/%u) (%s) %s...",
                    ++index,
                    total,
                    test->file,
                    test->name);

            fflush(stdout);

            test->entry();
            printf("OK\n");
        }
    }

    (void)argc; (void)argv;
    return 0;
}
