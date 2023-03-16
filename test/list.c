/////////////////////////////////////////////////////////////////////////////
//
// Terrascale
// Copyright (c) Dave Kilian & Emily Kilian 2023. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////

#include <pch.h>
#include <test/test.h>
#include <runtime/list.h>

static void validate_list_structure(trlist *list)
{
    trlist *node = list;
    do {

        TEST_EQUAL(node->next->prev, node);
        TEST_EQUAL(node->prev->next, node);

        node = node->next;

    } while (node != list);
}

static void list_staticinit()
{
    trlist list = tr_list_staticinit(list);

    TEST_EQUAL(list.next, &list);
    TEST_EQUAL(list.prev, &list);
}

static void list_initialize()
{
    trlist list;
    memset(&list, 0, sizeof(list));

    tr_list_initialize(&list);

    TEST_EQUAL(list.next, &list);
    TEST_EQUAL(list.prev, &list);
}

static void list_empty()
{
    trlist list = tr_list_staticinit(list);

    TEST_TRUE(tr_list_empty(&list));

    trlist item;
    list.next = &item; item.prev = &list;
    list.prev = &item; item.next = &list;

    TEST_FALSE(tr_list_empty(&list));
}

static void list_prepend()
{
    trlist list = tr_list_staticinit(list);
    trlist item1 = tr_list_staticinit(list);
    trlist item2 = tr_list_staticinit(list);

    TEST_TRUE(tr_list_empty(&list));

    tr_list_prepend(&list, &item1);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &list);

    tr_list_prepend(&list, &item2);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item2);
    TEST_EQUAL(list.next->next, &item1);
    TEST_EQUAL(list.next->next->next, &list);
}

static void list_append()
{
    trlist list = tr_list_staticinit(list);
    trlist item1 = tr_list_staticinit(list);
    trlist item2 = tr_list_staticinit(list);

    TEST_TRUE(tr_list_empty(&list));

    tr_list_append(&list, &item1);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &list);

    tr_list_append(&list, &item2);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &item2);
    TEST_EQUAL(list.next->next->next, &list);
}

static void list_remove()
{
    trlist list = tr_list_staticinit(list);
    trlist item1 = tr_list_staticinit(list);
    trlist item2 = tr_list_staticinit(list);
    trlist item3 = tr_list_staticinit(list);

    tr_list_append(&list, &item1);
    tr_list_append(&list, &item2);
    tr_list_append(&list, &item3);

    validate_list_structure(&list);

    tr_list_remove(&item2);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &item3);
    TEST_EQUAL(list.next->next->next, &list);

    tr_list_remove(&item3);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &list);

    tr_list_remove(&item1);

    validate_list_structure(&list);
    TEST_TRUE(tr_list_empty(&list));
}

static void list_rmhead()
{
    trlist list = tr_list_staticinit(list);
    trlist item1 = tr_list_staticinit(list);
    trlist item2 = tr_list_staticinit(list);
    trlist item3 = tr_list_staticinit(list);

    tr_list_append(&list, &item1);
    tr_list_append(&list, &item2);
    tr_list_append(&list, &item3);

    validate_list_structure(&list);

    trlist *removed = tr_list_rmhead(&list);
    TEST_EQUAL(removed, &item1);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item2);
    TEST_EQUAL(list.next->next, &item3);
    TEST_EQUAL(list.next->next->next, &list);

    TEST_EQUAL(tr_list_rmhead(&list), &item2);
    TEST_EQUAL(tr_list_rmhead(&list), &item3);
    TEST_NULL(tr_list_rmhead(&list));
}

static void list_rmtail()
{
    trlist list = tr_list_staticinit(list);
    trlist item1 = tr_list_staticinit(list);
    trlist item2 = tr_list_staticinit(list);
    trlist item3 = tr_list_staticinit(list);

    tr_list_append(&list, &item1);
    tr_list_append(&list, &item2);
    tr_list_append(&list, &item3);

    validate_list_structure(&list);

    trlist *removed = tr_list_rmtail(&list);
    TEST_EQUAL(removed, &item3);

    validate_list_structure(&list);
    TEST_FALSE(tr_list_empty(&list));
    TEST_EQUAL(list.next, &item1);
    TEST_EQUAL(list.next->next, &item2);
    TEST_EQUAL(list.next->next->next, &list);

    TEST_EQUAL(tr_list_rmtail(&list), &item2);
    TEST_EQUAL(tr_list_rmtail(&list), &item1);
    TEST_NULL(tr_list_rmhead(&list));
}

static void list_concatenate()
{
    trlist list1 = tr_list_staticinit(list1);
    trlist list2 = tr_list_staticinit(list2);

    trlist item1 = tr_list_staticinit(item1);
    trlist item2 = tr_list_staticinit(item2);
    trlist item3 = tr_list_staticinit(item3);
    trlist item4 = tr_list_staticinit(item4);
    trlist item5 = tr_list_staticinit(item5);
    trlist item6 = tr_list_staticinit(item6);

    tr_list_append(&list1, &item1);
    tr_list_append(&list1, &item2);
    tr_list_append(&list1, &item3);
    tr_list_append(&list2, &item4);
    tr_list_append(&list2, &item5);
    tr_list_append(&list2, &item6);

    validate_list_structure(&list1);
    validate_list_structure(&list2);

    tr_list_concatenate(&list2, &list1);

    validate_list_structure(&list1);
    validate_list_structure(&list2);

    TEST_FALSE(tr_list_empty(&list1));
    TEST_TRUE(tr_list_empty(&list2));

    TEST_EQUAL(list1.next, &item1);
    TEST_EQUAL(list1.next->next, &item2);
    TEST_EQUAL(list1.next->next->next, &item3);
    TEST_EQUAL(list1.next->next->next->next, &item4);
    TEST_EQUAL(list1.next->next->next->next->next, &item5);
    TEST_EQUAL(list1.next->next->next->next->next->next, &item6);
    TEST_EQUAL(list1.next->next->next->next->next->next->next, &list1);
}

static void list_transfer()
{
    trlist list1 = tr_list_staticinit(list1);
    trlist list2 = tr_list_staticinit(list2);

    trlist item1 = tr_list_staticinit(item1);
    trlist item2 = tr_list_staticinit(item2);
    trlist item3 = tr_list_staticinit(item3);
    trlist item4 = tr_list_staticinit(item4);
    trlist item5 = tr_list_staticinit(item5);
    trlist item6 = tr_list_staticinit(item6);

    tr_list_append(&list1, &item1);
    tr_list_append(&list1, &item2);
    tr_list_append(&list1, &item3);
    tr_list_append(&list1, &item4);
    tr_list_append(&list1, &item5);
    tr_list_append(&list1, &item6);

    validate_list_structure(&list1);
    validate_list_structure(&list2);

    TEST_FALSE(tr_list_empty(&list1));
    TEST_TRUE(tr_list_empty(&list2));

    tr_list_transfer(&list1, &list2);

    validate_list_structure(&list1);
    validate_list_structure(&list2);

    TEST_TRUE(tr_list_empty(&list1));
    TEST_FALSE(tr_list_empty(&list2));

    TEST_EQUAL(list2.next, &item1);
    TEST_EQUAL(list2.next->next, &item2);
    TEST_EQUAL(list2.next->next->next, &item3);
    TEST_EQUAL(list2.next->next->next->next, &item4);
    TEST_EQUAL(list2.next->next->next->next->next, &item5);
    TEST_EQUAL(list2.next->next->next->next->next->next, &item6);
    TEST_EQUAL(list2.next->next->next->next->next->next->next, &list2);
}

static void list_foreach()
{
    trlist list = tr_list_staticinit(list);

    trlist item1 = tr_list_staticinit(item1);
    trlist item2 = tr_list_staticinit(item2);
    trlist item3 = tr_list_staticinit(item3);
    trlist item4 = tr_list_staticinit(item4);
    trlist item5 = tr_list_staticinit(item5);
    trlist item6 = tr_list_staticinit(item6);

    tr_list_append(&list, &item1);
    tr_list_append(&list, &item2);
    tr_list_append(&list, &item3);
    tr_list_append(&list, &item4);
    tr_list_append(&list, &item5);
    tr_list_append(&list, &item6);

    validate_list_structure(&list);

    trlist *items[] = { &item1, &item2, &item3, &item4, &item5, &item6 };

    int index = 0;
    tr_list_foreach(&list, entry) {
        TEST_EQUAL(entry, items[index++]);
    }

    TEST_EQUAL(arraysize(items), index);
}

static void list_rforeach()
{
    trlist list = tr_list_staticinit(list);

    trlist item1 = tr_list_staticinit(item1);
    trlist item2 = tr_list_staticinit(item2);
    trlist item3 = tr_list_staticinit(item3);
    trlist item4 = tr_list_staticinit(item4);
    trlist item5 = tr_list_staticinit(item5);
    trlist item6 = tr_list_staticinit(item6);

    tr_list_append(&list, &item1);
    tr_list_append(&list, &item2);
    tr_list_append(&list, &item3);
    tr_list_append(&list, &item4);
    tr_list_append(&list, &item5);
    tr_list_append(&list, &item6);

    validate_list_structure(&list);

    trlist *items[] = { &item1, &item2, &item3, &item4, &item5, &item6 };

    int index = arraysize(items);;
    tr_list_rforeach(&list, entry) {
        TEST_EQUAL(entry, items[--index]);
    }

    TEST_EQUAL(0, index);
}

static void slist_staticinit()
{
    trslist slist = tr_slist_staticinit;
    TEST_NULL(slist.next);
}

static void slist_initialize()
{
    trslist slist;
    tr_slist_initialize(&slist);
    TEST_NULL(slist.next);
}

static void slist_empty()
{
    trslist slist = tr_slist_staticinit;
    trslist item1 = tr_slist_staticinit;

    TEST_TRUE(tr_slist_empty(&slist));

    slist.next = &item1;

    TEST_FALSE(tr_slist_empty(&slist));
}

static void slist_push()
{
    trslist slist = tr_slist_staticinit;
    trslist item1 = tr_slist_staticinit;
    trslist item2 = tr_slist_staticinit;

    TEST_TRUE(tr_slist_empty(&slist));

    tr_slist_push(&slist, &item2);

    TEST_FALSE(tr_slist_empty(&slist));
    TEST_EQUAL(slist.next, &item2);
    TEST_EQUAL(item2.next, NULL);

    tr_slist_push(&slist, &item1);

    TEST_FALSE(tr_slist_empty(&slist));
    TEST_EQUAL(slist.next, &item1);
    TEST_EQUAL(item1.next, &item2);
    TEST_EQUAL(item2.next, NULL);
}

static void slist_pop()
{
    trslist slist = tr_slist_staticinit;
    trslist item1 = tr_slist_staticinit;
    trslist item2 = tr_slist_staticinit;
    trslist item3 = tr_slist_staticinit;

    tr_slist_push(&slist, &item3);
    tr_slist_push(&slist, &item2);
    tr_slist_push(&slist, &item1);

    TEST_FALSE(tr_slist_empty(&slist));
    TEST_EQUAL(slist.next, &item1);
    TEST_EQUAL(item1.next, &item2);
    TEST_EQUAL(item2.next, &item3);
    TEST_EQUAL(item3.next, NULL);

    TEST_EQUAL(tr_slist_pop(&slist), &item1);

    TEST_FALSE(tr_slist_empty(&slist));
    TEST_EQUAL(slist.next, &item2);
    TEST_EQUAL(item2.next, &item3);
    TEST_EQUAL(item3.next, NULL);

    TEST_EQUAL(tr_slist_pop(&slist), &item2);

    TEST_FALSE(tr_slist_empty(&slist));
    TEST_EQUAL(slist.next, &item3);
    TEST_EQUAL(item3.next, NULL);

    TEST_EQUAL(tr_slist_pop(&slist), &item3);

    TEST_TRUE(tr_slist_empty(&slist));

    TEST_NULL(tr_slist_pop(&slist));
}

static void slist_foreach()
{
    trslist slist = tr_slist_staticinit;
    trslist item1 = tr_slist_staticinit;
    trslist item2 = tr_slist_staticinit;
    trslist item3 = tr_slist_staticinit;

    tr_slist_push(&slist, &item3);
    tr_slist_push(&slist, &item2);
    tr_slist_push(&slist, &item1);

    trslist *items[] = { &item1, &item2, &item3 };

    int index = 0;
    tr_slist_foreach(&slist, entry) {
        TEST_EQUAL(entry, items[index++]);
    }

    TEST_EQUAL(arraysize(items), index);
}

static const test_case list_cases[] =
{
    TEST_CASE(list_staticinit),
    TEST_CASE(list_initialize),
    TEST_CASE(list_empty),
    TEST_CASE(list_prepend),
    TEST_CASE(list_append),
    TEST_CASE(list_remove),
    TEST_CASE(list_rmhead),
    TEST_CASE(list_rmtail),
    TEST_CASE(list_concatenate),
    TEST_CASE(list_transfer),
    TEST_CASE(list_foreach),
    TEST_CASE(list_rforeach),

    TEST_CASE(slist_staticinit),
    TEST_CASE(slist_initialize),
    TEST_CASE(slist_empty),
    TEST_CASE(slist_push),
    TEST_CASE(slist_pop),
    TEST_CASE(slist_foreach),
};

TEST_SUITE(list_tests, list_cases);
