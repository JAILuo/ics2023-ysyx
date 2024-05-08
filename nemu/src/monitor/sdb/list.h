#ifndef _lcthw_List_h
#define _lcthw_List_h

#include <stdlib.h>

struct ListNode;

typedef struct ListNode
{
    struct ListNode *next;
    struct ListNode *prev;
    void *value;
} ListNode;

typedef struct List
{
    int count;
    ListNode *first;
    ListNode *last;
} List;

List *List_create();

/*
 * Destroy the linked list and all the elements it contains.
 */
void List_destroy(List *list);

/*
 * Release the value in each node (not the node itself)
 */
void List_clear(List *list);

/*
 * Clean up and destroy linked lists (Each element in the linked list is traversed twice)
 */
void List_clear_destroy(List *list);

#define List_count(A) ((A)->count)
#define List_first(A) ((A)->first != NULL ? (A)->first->value : NULL)
#define List_last(A) ((A)->last != NULL ? (A)->last->value : NULL)

/*
 * Add a element to the tail of the linked list
 */
void List_push(List *list, void *value);

/*
 * Remove the element from the end of the linked list
 */
void *List_pop(List *list);

/*
 * Quickly add elements to the header of a linked list
 */
void List_unshift(List *list, void *value);

/*
 * Remove the first element of the linked list
 */
void *List_shift(List *list);

/*
 * Delete an element in a linked list
 */
void *List_remove(List *list, ListNode *node);

/*
 * Loop through the common structure of a linked list
 */
#define LIST_FOREACH(L, S, M, V) \
    ListNode *_node = NULL;      \
    ListNode *V = NULL;          \
    for (V = _node = L->S; _node != NULL; V = _node = _node->M)

#endif
