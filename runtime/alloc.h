//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.  
//
// alloc.h - a tagging heap allocator
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Identifies the subsystem which made a heap allocation.
// Use a four-char literal (like 'abcd') to identify a subsystem;
// or, specify 0 to make an untagged allocation.
//
typedef uint32_t tralloctag;

// Converts an alloc tag of the form 'abcd' to the string "abcd"
// The fifth character of the output string stores a null terminator.
//
void tr_alloctag_tostr(tralloctag tag, char str[5]);

// A malloc() replacement that tags memory
void *tr_alloc(unsigned bytes, tralloctag tag);

// Frees tr_alloc-allocated memory
void tr_free(void *block);

// Statistics about memory allocations for a given tag
typedef struct {

    tralloctag tag;  // Which tag this structure measures
    unsigned nalloc; // Number of unique heap allocations
    unsigned nbytes; // Number of heap bytes

} trallocstat;

// Gets the current allocation statistics for a single tag
trallocstat tr_alloc_stat(tralloctag tag);

// Gets allocation statistics for all tags
//
// This routine returns statistics for as many tags as possible with the
// buffer provided, then returns the number of tags for which statistics are
// currently available.
//
// If the count returned is greater than the size of your buffer, then you
// received an arbitrary subset of the tag statistics; you may want to alloc
// a larger buffer and try again.
//
// If the count returned is less than the size of your buffer, then only that
// many elements of your buffer were filled; the rest were left uninitialized.
//
int tr_alloc_stats(trallocstat *buffer, int count);
