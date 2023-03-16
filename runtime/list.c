//////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <runtime/list.h>

#define tr_assert_link_valid(from, to) \
    do { \
        tr_assert((from)->next == (to)); \
        tr_assert((to)->prev == (from)); \
    } while (0)

#define tr_assert_node_valid(item) \
    do { \
        tr_assert_link_valid(item, item->next); \
        tr_assert_link_valid(item->prev, item); \
    } while (0)

void tr_list_initialize(trlist *list)
{
    list->next = list;
    list->prev = list;
}

bool tr_list_empty(trlist *list)
{
    tr_assert_node_valid(list);
    tr_assert((list->next == list) == (list->prev == list));

    return list->next == list;
}

void tr_list_prepend(trlist *list, trlist *item)
{
    tr_assert_node_valid(list);
    tr_assert_node_valid(list->next);

    trlist *head = list->next;

    list->next = item;
    item->prev = list;

    item->next = head;
    head->prev = item;
}

void tr_list_append(trlist *list, trlist *item)
{
    tr_assert_node_valid(list);
    tr_assert_node_valid(list->prev);

    trlist *tail = list->prev;

    list->prev = item;
    item->next = list;

    item->prev = tail;
    tail->next = item;
}

void tr_list_remove(trlist *item)
{
#if DEBUG && !NDEBUG
    tr_assert_node_valid(item);
    if (!tr_list_empty(item)) {
        tr_assert_node_valid(item->prev);
        tr_assert_node_valid(item->next);
    }
#endif

    trlist *prev = item->prev;
    trlist *next = item->next;

    prev->next = next;
    next->prev = prev;

    item->prev = item;
    item->next = item;
}

trlist *tr_list_rmhead(trlist *list)
{
    tr_assert_node_valid(list);
    tr_assert_node_valid(list->next);

    if (tr_list_empty(list)) {
        return NULL;
    }

    trlist *head = list->next;
    tr_list_remove(head);
    return head;
}

trlist *tr_list_rmtail(trlist *list)
{
    tr_assert_node_valid(list);
    tr_assert_node_valid(list->prev);

    if (tr_list_empty(list)) {
        return NULL;
    }

    trlist *tail = list->prev;
    tr_list_remove(tail);
    return tail;
}

void tr_list_concatenate(trlist *source, trlist *target)
{
    tr_assert_node_valid(source);
    tr_assert_node_valid(source->prev);
    tr_assert_node_valid(source->next);

    tr_assert_node_valid(target);
    tr_assert_node_valid(target->prev);
    tr_assert_node_valid(target->next);

    if (!tr_list_empty(source)) {

        trlist *head = source->next; source->next = source;
        trlist *tail = source->prev; source->prev = source;

        target->prev->next = head;
        head->prev = target->prev;

        target->prev = tail;
        tail->next = target;
    }
}

void tr_list_transfer(trlist *source, trlist *target)
{
    tr_assert(tr_list_empty(target));
    tr_list_concatenate(source, target);
}

void tr_slist_initialize(trslist *list)
{
    list->next = NULL;
}

bool tr_slist_empty(trslist *list)
{
    return list->next == NULL;
}

void tr_slist_push(trslist *list, trslist *item)
{
    item->next = list->next;
    list->next = item;
}

trslist *tr_slist_pop(trslist *list)
{
    trslist *head = list->next;
    if (head == NULL) {
        return NULL;
    }

    list->next = head->next;
    return head;
}
