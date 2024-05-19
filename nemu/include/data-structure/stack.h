#ifndef _STACK_H
#define _STACK_H

#include "list.h"


/*
 * Improvement: Replace the List with DArray
 */
typedef List Stack;


#define Stack_create() List_create()

#define Stack_destroy(stack) List_destroy(stack)

#define Stack_push(stack, value) List_push(stack, value)

#define Stack_pop(stack) List_pop(stack)


#define Stack_count(A) (A->count)
#define Stack_peek(A) (A->last != NULL ? A->last->value : NULL)


#define STACK_FOREACH(stack, cur) \
    LIST_FOREACH( stack, first, next, cur )


#endif

