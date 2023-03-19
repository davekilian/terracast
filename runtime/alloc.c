//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <runtime/alloc.h>
#include <runtime/list.h>

void tr_alloctag_tostr(tralloctag tag, char str[5])
{
    char *bytes = (void *)&tag;
    for (int i = 0; i < 4; ++i) {
        str[i] = bytes[3 - i];
    }

    str[4] = 0;
}

// A hash table bucket of tag statistics
typedef struct {

    pthread_mutex_t lock; // Locks this bucket's entry list
    trlist entries;       // Chain of statentry items

} statbucket;

// Hash table entry with statistics for a single tag
typedef struct {

    trlist entry;       // Entry in statbucket.entries
    trallocstat stat;   // Statistics for this tag

} statentry;

static statbucket statbuckets[16];           // Global table of tag statistics
pthread_once_t statinit = PTHREAD_ONCE_INIT; // Initialization state

// Initializes the statbuckets table from a pthread_once() call
static void tr_alloc_init_stats()
{
    for (int i = 0; i < arraysize(statbuckets); ++i) {
        statbucket *bucket = statbuckets + i;
        pthread_mutex_init(&bucket->lock, NULL);
        tr_list_initialize(&bucket->entries);
    }
}

// Locks the right hash table bucket and returns the entry for the given tag,
// allocating a new statentry for the given tag on-the-fly if needed
//
static void tr_alloc_lock_tag(
        tralloctag tag,
        statbucket **bucket,
        statentry **st)
{
    unsigned index = tag & (arraysize(statbuckets) - 1);
    statbucket *b = statbuckets + index;
    pthread_mutex_lock(&b->lock);

    tr_list_foreach(&b->entries, item) {
        statentry *entr = container_of(item, statentry, entry);
        if (entr->stat.tag == tag) {
            tr_list_remove(&entr->entry);
            tr_list_prepend(&b->entries, &entr->entry);
            *bucket = b;
            *st = entr;
            return;
        }
    }

    statentry *entr = malloc(sizeof(statentry));
    tr_require(entr != NULL && "out of memory for statentry");

    tr_list_initialize(&entr->entry);
    entr->stat.tag = tag;
    entr->stat.nalloc = 0;
    entr->stat.nbytes = 0;

    tr_list_prepend(&b->entries, &entr->entry);
    *bucket = b;
    *st = entr;
}

static void tr_alloc_unlock_tag(
        statbucket *bucket,
        statentry *entry)
{
    pthread_mutex_unlock(&bucket->lock);
    (void)entry;
}

// Header inserted before the user region of a heap allocation
typedef struct {

    tralloctag tag;   // User-provided allocation tag
    unsigned bytes;   // Number of bytes allocated
    char reserved[8]; // For alignment

} allochdr;

//
// On 64-bit architectures, callers can reasonably assume malloc() will return
// 16-byte-aligned memory, in order to support CPU instructions that involve
// 32-bit half-words, 64-bit full words, and 128-bit double-words. Beyond that,
// for very-wide alignment needs as seen in SIMD instructions, callers usually
// expect to need to align memory themselves.
//
// Our allocation headers should be exactly 16 bytes long so we can preserve
// 16-byte alignment guarantees callers reasonably expect from malloc().
//
static_assert(sizeof(allochdr) == 16);

void *tr_alloc(unsigned bytes, tralloctag tag)
{
    void *mem = malloc(bytes + sizeof(allochdr));
    if (mem == NULL) {
        return NULL;
    }

    allochdr *hdr = mem;
    hdr->tag = tag;
    hdr->bytes = bytes;

    tr_require(0 == pthread_once(&statinit, &tr_alloc_init_stats));

    statbucket *bucket; statentry *stat;
    tr_alloc_lock_tag(tag, &bucket, &stat);
    stat->stat.nalloc += 1;
    stat->stat.nbytes += bytes;
    tr_alloc_unlock_tag(bucket, stat);

    return hdr + 1;
}

void tr_free(void *block)
{
    allochdr *hdr = ptr_sub(block, sizeof(allochdr));

    tr_require(0 == pthread_once(&statinit, &tr_alloc_init_stats));

    statbucket *bucket; statentry *stat;
    tr_alloc_lock_tag(hdr->tag, &bucket, &stat);

    tr_assert(stat->stat.nalloc > 0);
    tr_assert(stat->stat.nbytes >= hdr->bytes);

    stat->stat.nalloc -= 1;
    stat->stat.nbytes -= hdr->bytes;

    tr_alloc_unlock_tag(bucket, stat);
    free(hdr);
}

trallocstat tr_alloc_stat(tralloctag tag)
{
    tr_require(0 == pthread_once(&statinit, &tr_alloc_init_stats));

    statbucket *bucket; statentry *stat;
    tr_alloc_lock_tag(tag, &bucket, &stat);
    trallocstat result = stat->stat;
    tr_alloc_unlock_tag(bucket, stat);

    return result;
}

int tr_alloc_stats(trallocstat *buffer, int count)
{
    tr_require(0 == pthread_once(&statinit, &tr_alloc_init_stats));

    int total = 0;

    for (int i = 0; i < arraysize(statbuckets); ++i) {
        pthread_mutex_lock(&statbuckets[i].lock);
    }

    for (int i = 0; i < arraysize(statbuckets); ++i) {
        statbucket *bucket = statbuckets + i;
        tr_list_foreach(&bucket->entries, item) {
            total += 1;
            if (count > 0) {
                statentry *entry = container_of(item, statentry, entry);
                *buffer = entry->stat;
                buffer += 1;
                count -= 1;
            }
        }
    }

    for (int i = 0; i < arraysize(statbuckets); ++i) {
        pthread_mutex_unlock(&statbuckets[i].lock);
    }

    return total;
}
