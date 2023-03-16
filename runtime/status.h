//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// status.h - status codes and error handling
//
// The trstatus type is a unified status code that multiplexes status codes
// obtained from external subsystems. A few of the bits are reserved for
// indicating which subsystem the status code came from, and the rest of the
// bits are the original status code itself. The design is inspired by
// Windows's HRESULT type.
//
// A table of 'native' error codes that come from Terrascale itself is
// defined at the end of this file.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// A unified error code type
//
// The high bit (trstatus_fail_mask) indicates success or failure
// The next 7 bits (trstatus_origin_mask) tell where the error came from
// The remaining bits contain the original status code
//
typedef uint32_t trstatus;

#define trstatus_fail_mask   0x80000000 /* The success/failure bit */
#define trstatus_origin_mask 0x7f000000 /* A trstatus_origin value */
#define trstatus_code_mask   0x00ffffff /* The original error code */

// Subsystems that can generate status codes stored in a trstatus
typedef enum {

    trstatus_native = 0x00000000, // Terrascale error code types
    trstatus_errno  = 0x01000000, // The Posix 'errno' variable
    trstatus_http   = 0x02000000, // HTTP status codes

} trstatus_origin;

// Constructs a trstatus value from its components
//
// success: 0 for failure or 1 for success
// origin: a trstatus_origin value
// code: the original status code
//
#define tr_status(success, origin, code) \
    ((trstatus) \
    ((((uint32_t)(!((bool)(success)))) * trstatus_fail_mask) | \
     (((uint32_t)(origin)) & trstatus_origin_mask) | \
     (((uint32_t)(code)) & trstatus_code_mask)))

// Constructs a trstatus from a given errno value
//
// The errno provided should be positive (>= 0); if you're using a Posix API
// that returns a positive number on success or a negative errno on failure,
// negate the error code being returned to this function.
//
// int nwrite = write(...);
// if (nwrite < 0) {
//     return tr_status_from_errno_value(-nwrite); // <-- negate nwrite
// }
//
#define tr_status_from_errno_value(e) \
    tr_status(e == 0, trstatus_errno, e)

// Constructs a trstatus from the current value of the errno variable
#define tr_status_from_errno() \
    tr_status_from_errno_value(errno)

// Constructs a trstatus for the given HTTP status code
#define tr_status_from_http(status) \
    tr_status(status / 100 == 2, trstatus_http, status)

// Indicates whether the given status is successful
#define tr_ok(status) (((status) & trstatus_fail_mask) == 0)

// Indicates whether the given status is for a failure
#define tr_failed(status) (!tr_ok(status))

#define trstatus_ok        tr_status(1, trstatus_native,  0) /* success */
#define trstatus_fail      tr_status(0, trstatus_native,  0) /* unspecified error */
#define trstatus_pending   tr_status(0, trstatus_native,  1) /* operation started */
#define trstatus_no_mem    tr_status(0, trstatus_native,  2) /* out of memory */
#define trstatus_assert    tr_status(0, trstatus_native,  3) /* assert failure */
#define trstatus_nyi       tr_status(0, trstatus_native,  4) /* not yet implemented */
#define trstatus_exists    tr_status(0, trstatus_native,  5) /* target already exists */
#define trstatus_not_found tr_status(0, trstatus_native,  6) /* target not found */
#define trstatus_empty     tr_status(0, trstatus_native,  7) /* expected non-empty collection */
#define trstatus_argument  tr_status(0, trstatus_native,  8) /* invalid argument */
#define trstatus_shutdown  tr_status(0, trstatus_native,  9) /* target shutdown in progress */
#define trstatus_too_small tr_status(0, trstatus_native, 10) /* given value not big enough */
#define trstatus_too_large tr_status(0, trstatus_native, 11) /* given value too big */
#define trstatus_parse     tr_status(0, trstatus_native, 12) /* failed to parse string */
#define trstatus_no_more   tr_status(0, trstatus_native, 13) /* no more items in collection */
#define trstatus_version   tr_status(0, trstatus_native, 14) /* unsupported version number */
#define trstatus_later     tr_status(0, trstatus_native, 15) /* retry orperation later */
#define trstatus_expired   tr_status(0, trstatus_native, 16) /* target no longer available */
#define trstatus_support   tr_status(0, trstatus_native, 17) /* feature not supported */
#define trstatus_overrun   tr_status(0, trstatus_native, 18) /* I/O access out of bounds */
#define trstatus_async     tr_status(0, trstatus_native, 19) /* operation in progress */
