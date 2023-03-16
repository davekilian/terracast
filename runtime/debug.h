//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// debug.h - debugging utilities and macros
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <runtime/status.h>

// Dumps a message to stderr. Use only for exceptional circumstances
#define tr_log(...) \
        do { \
            fprintf(stderr, "" __VA_ARGS__); \
            fprintf(stderr, " [%s:%d]\n", __FILE__, __LINE__); \
        } while (0)

// Issues a breakpoint instruction to the operating system.
//
// - If a debugger is attached, this will break into the debugger
// - Otherwise, this will terminate the calling process
//
void tr_break();

// Panics the calling process as the result of a serious error.
//
// Internally, this stops the process using the tr_break routine, which
// breaks into the debugger if one is attached. From the debugger, the user
// may choose to continue execution past the panic if they wish. If no
// debugger is attached, however, panicking with this routine terminates
// the calling process.
//
#define tr_panic(reason) \
    tr_do_panic(trstatus_assert, __FILE__, __LINE__, "" reason)

void tr_do_panic(trstatus s, const char *file, int line, const char *expr);

// Panics if the given expression is false, even on non-debug builds
#define tr_require(expr) \
    do { \
        if (!(expr)) { \
            tr_panic("Assertion failed: " #expr); \
        } \
    } while (0)

// Panics if the given expression is false on debug-builds.
// Compiled out on non-debug builds.
#if DEBUG && !NDEBUG
#define tr_assert(expr) tr_require(expr)
#else
#define tr_assert(expr)
#endif
