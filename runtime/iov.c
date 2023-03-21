//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>

#include <runtime/iov.h>
#include <runtime/stack.h>

//
// TODO
//
// struct iovec is ptr iov_base and size_t iov_len
// you include it from sys/uio.h
// readv takes a const struct iov pointer and iovcnt
// the length of the read is implicit in the iov size
//
