//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// macros.h - basic utility macros
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// Swaps two variables of type 'type'
#define swap(type, a, b) \
    do { \
        type __temp = a; \
        a = b; \
        b = __temp; \
    } while (0)

// Calculates the number of elements in a static array
#define arraysize(array) ((ssize_t)(sizeof(array) / sizeof((array)[0])))

// Returns the minimum of two numerical values
#define min(a, b) (((a) < (b)) ? (a) : (b))

// Returns the maximum of two numerical values
#define max(a, b) (((a) > (b)) ? (a) : (b))

// Returns the absolute value of the given numerical value
#define abs(a) ((a) * (((a) < 0) ? -1 : 1))

// Gets a pointer `incr` bytes greater than `ptr`
#define ptr_add(ptr, incr) ((void *)(((char *)(ptr)) + (incr)))

// Gets a pointer `decr` bytes less than `ptr`
#define ptr_sub(ptr, decr) ((void *)(((char *)(ptr)) - (decr)))

// Computes the number of bytes between pointers `from` and `to`
#define ptr_dist(from, to) ((long)(((char *)(to)) - ((char *)(from))))

// Returns the given pointer plus the minimal numer of bytes needed to align
// to the given alignment, which must be a power of two.
//
#define ptr_align(ptr, align) ((void *)(((uintptr_t)ptr_add(ptr, align)) & ~((align) - 1)))

// Returns the offset in bytes of the given field of the given structure
#define field_offset(type, field) offsetof(type, field)

// Gets the size in bytes of the given field of the given structure
#define field_size(type, field) sizeof(((type *)NULL)->field)

// Given a pointer `var` which is a field named `field` of struct `type`,
// computes a pointer to the given structure. For example:
//
//  typedef struct {
//
//      int field1;
//      int field2;
//      int field3;
//
//  } mystruct;
//
//  mystruct s;
//  void *fieldptr = &s.field2;
//
//  void *structptr = container_of(
//      fieldptr, // pointer to a field of a structure
//      mystruct, // type name of the outer structure
//      field2);  // which field of that structure is pointed to
//  assert(structptr == &s); // the outer structure was returned
//
#define container_of(var, type, field) \
    ((type *)ptr_sub(var, field_offset(type, field)))

// The to_string(x) macro evaluates x and converts it to a string constant.
//
// For example, __LINE__ evaluates to an integer literal giving the current
// line number of the code (e.g.: 23). TOSTRING(__LINE__) evaluates __LINE__
// and produces a string literal (e.g. "23"). This can be useful for string
// literal concatenation at compile time.
//
#define __to_string(x) #x
#define to_string(x) __to_string(x)
