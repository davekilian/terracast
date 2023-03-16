//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
// list.h - singly- and doubly-linked lists
//
// These list types are 'intrusive,' meaning you embed the trlist or trslist
// structures directly inside your structures. To link your structure into a
// list, you call a list append/prepend/etc routine, passing in a pointer to
// your structure's list node. When iterating through the list, use the
// container_of macro to recover your structure pointer from a given list
// node pointer.
//
// The trlist and trslist types are loosely based off Windows's LIST_ENTRY
// and SLIST_ENTRY types. You can find further docs for those online.
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

struct _trslist;
struct _trlist;

// A node in a doubly-linked list.
//
// Each link in the list tracks both a forward pointer to the next entry, and
// a back pointer to the previous entry. The list itself is represented as a
// sentinel node; the sentinel's 'next' pointer is the head of the list, and
// the sentinel's 'previous' pointer is the tail of the list. If the list is
// empty, the sentinel node points forwards and backwards to itself. With this
// construction, prepend (insert at head) and append (insert are tail) are
// both very simple constant-time operations.
//
typedef struct _trlist {

    struct _trlist *next; // The next link in the list
    struct _trlist *prev; // The previous link in the list

} trlist;

// Static initializer for a list or a list entry.
//
// Because empty list nodes point forward and backward to themselves, this
// macro accepts the variable being initialized as input. For example:
//
//    static trlist mylist = tr_list_staticinit(mylist);
//
#define tr_list_staticinit(varname) \
    { .next = &(varname), .prev = &(varname) }

// Explicitly initializes the given list node
//
// This has the same effect as tr_list_staticinit, but can be used in
// situations where it's not possible to use the static initializer (e.g.
// when initializing a heap-allocated structure which contains an trlist
// as a field).
//
void tr_list_initialize(trlist *list);

// Indicates whether there are any entries in the given list
bool tr_list_empty(trlist *list);

// Adds the given item to the beginning of the list
void tr_list_prepend(trlist *list, trlist *item);

// Adds the given item to the end of the list
void tr_list_append(trlist *list, trlist *item);

// Removes the given item from the list
void tr_list_remove(trlist *item);

// Removes the first item from the list and returns it.
// If the given list is empty, this returns NULL.
//
trlist *tr_list_rmhead(trlist *list);

// Removes the last item from the list and returns it.
// If the given list is empty, this returns NULL.
//
trlist *tr_list_rmtail(trlist *list);

// Transfers all entries in the source list and appends them to the end of
// the target list.
//
// For example, if the lists initially contain the following entries:
//
// - target: A B C
// - source: D E F
//
// Then after this operation, the lists look as follows:
//
// - target: A B C D E F
// - source: <empty>
//
void tr_list_concatenate(trlist *source, trlist *target);

// Transfers all the source list's entries to the target list.
// The target list must empty; otherwise, the list will be corrupted.
//
// Internally, this is just an tr_list_concatenate after verification
// the target list is empty. This routine was provided separately to
// help make calling code clearer.
//
void tr_list_transfer(trlist *source, trlist *target);

// Iterates over each entry in list. The second argument to this macro gives
// the name of the iteration variable defined inline in the for loop. For
// example:
//
//      tr_list_foreach(mylist, myentry) {
//          mystruct *s = container_of(myentry, mystruct, entry);
//          /* do something with s */
//      }
//
// Do not modify the iteration variable (`myentry` in the example) while
// the list is still being iterated.
//
#define tr_list_foreach(list, item) \
    for (trlist *item = (list)->next; \
         item != (list); \
         item = item->next)

// Iterates over the given list in reverse
#define tr_list_rforeach(list, item) \
    for (trlist *item = (list)->prev; \
         item != (list); \
         item = item->prev)

// A node in a singly-linked list.
//
// Each link in an slist only tracks the pointer to the next object. Because
// the tail of the list cannot be found in constant time, slist only supports
// modification at the head of the list (push/pop).
//
typedef struct _trslist {

    struct _trslist *next; // The next node in the list

} trslist;

// Static initializer for a list or a list entry
#define tr_slist_staticinit { .next = NULL }

// Explicitly initializes the given list node
//
// This has the same effect as tr_slist_staticinit, but can be used in
// situations where it's not possible to use the static initializer (e.g.
// when initializing a heap-allocated structure which contains an tr_list
// as a field).
//
void tr_slist_initialize(trslist *list);

// Indicates whether there are any entries in the given list
bool tr_slist_empty(trslist *list);

// Adds the given item to the beginning of the list
void tr_slist_push(trslist *list, trslist *item);

// Removes the first item from the list and returns it.
// If the given list is empty, this returns NULL.
trslist *tr_slist_pop(trslist *list);

// Iterates over each entry in list. The second argument to this macro gives
// the name of the iteration variable defined inline in the for loop. For
// example:
//
//      tr_slist_foreach(mylist, myentry) {
//          mystruct *s = container_of(myentry, mystruct, entry);
//          /* do something with s */
//      }
//
// Do not modify the iteration variable (`myentry` in the example) while
// the list is still being iterated.
//
#define tr_slist_foreach(list, item) \
    for (trslist *item = (list)->next; \
         item != NULL; \
         item = item->next)
