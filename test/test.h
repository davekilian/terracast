//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// test.h - a small test framework
//
// To add tests
//
// 1. Define a new module in this project (e.g. mysuite.c)
// 2. Define your tests in the module as static functions local to the module.
//    Every test is assumed passed if it returns control to the caller.
//    Use the TEST_ macros to assert and crash upon test failure.
//    Tests also fail if you or code you call crashes on tr_require/assert/etc.
// 3. Define a private, static array of tests using the TEST_CASE macro
// 4. Define a public, static test suite with the TEST_SUITE macro
// 5. In testmain.c, forward declare your test suite (as extern const)
// 6. Add your test to the test_suites array in testmain.c
//
// Use list.c as a relatively small example of a full test module.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Asserts the given expression resolves to a nonzero value
#define TEST_TRUE(expr) tr_require((expr) != false)

// Asserts the given expression resolves to zero
#define TEST_FALSE(expr) tr_require((expr) == false)

// Asserts the two expressions resolve to the same value
#define TEST_EQUAL(lhs, rhs) tr_require((lhs) == (rhs))

// Asserts the two expressions resolve to different values
#define TEST_NOT_EQUAL(lhs, rhs) tr_require((lhs) != (rhs))

// Asserts the given expression resolves to null
#define TEST_NULL(ptr) tr_require((ptr) == NULL)

// Asserts the given expression is non-null
#define TEST_NOT_NULL(ptr) tr_require((ptr) != NULL)

// Asserts the given tr::err code is success
#define TEST_SUCCESS(expr) do { \
        trstatus __e = (expr); \
        if (!tr_ok(__e)) { \
            tr_log("TEST_SUCCESS:[%s]=[0x%08x]", #expr, __e); \
            tr_panic(); \
        } \
    } while (0)

// Asserts the given tr::err code is failure
#define TEST_FAIL(expr) tr_require(!tr_ok(expr))

// Asserts the first argument is < the second
#define TEST_LESS_THAN(lhs, rhs) tr_require((lhs) < (rhs))

// Asserts the first argument is <= the second
#define TEST_LESS_EQUAL(lhs, rhs) tr_require((lhs) <= (rhs))

// Asserts the first argument is > the second
#define TEST_GREATER_THAN(lhs, rhs) tr_require((lhs) > (rhs))

// Asserts the first argument is >= the second
#define TEST_GREATER_EQUAL(lhs, rhs) tr_require((lhs) >= (rhs))

// Function signature for each test case this framework can execute
typedef void test_entry(void);

// Information about a test case
typedef struct {
    const char *name;   // The name of the entry point routine
    const char *file;   // File where the test is defined
    test_entry *entry;  // The test entry point routine
} test_case;

// Information about all test cases in a single code module
typedef struct {
    const test_case *cases; // Array of test cases
    int ncases;             // Number of cases in the array
} test_suite;

// Defines an entry in an array of test cases
#define TEST_CASE(func) { .name = #func, .file = __FILE__, .entry = &(func) }

// Locally defines a test suite with the given name and case array
#define TEST_SUITE(_name, _cases) \
        test_suite _name = { \
            .cases = (_cases), \
            .ncases = sizeof(_cases) / sizeof((_cases)[0]) \
        }
