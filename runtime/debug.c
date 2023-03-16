//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// debug.c - debugging utilities and macros
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>

#include <runtime/debug.h>

void tr_break()
{
     __asm volatile ("int3");
}

void tr_do_panic(trstatus s, const char *file, int line, const char *expr)
{
    tr_log("PANIC: %s:%d:0x%08x [%s]", file, line, s, expr);
    tr_break();
}
