// Copyright (C) 2012 Dennis Francis<dennisfrancis.in@gmail.com>

#include "dll.h"

#include <stdlib.h>
#include <string.h>


extern void n64_free(void *buf);
extern void *n64_malloc(size_t size_to_alloc);


static int __create_node( void *element, NODEptr *new_node );


/*
list_init()
-----------
Initializes a list structure
lst : pointer to list structure
comp_keys : pointer to a function which returns 0 when the keys of the arguments are same
*/
void list_init(list_t *lst, int (* comp_keys)(void *, void *))
{
    lst->head      = NULL;
    lst->count     = 0;
    lst->comp_keys = comp_keys;
}


/*
__create_node():
----------------
Creates a new node which encapsulates the data element given as argument

element : pointer to data element to be stored in the new node
new_node : pointer to node pointer variable where the allocated memory address is to
be stored.

*/
static int __create_node(void *element, NODEptr *new_node)
{
    *new_node = (NODEptr)n64_malloc(sizeof(NODE));

    if ( *new_node )
    {
        (*new_node)->element = element;
        (*new_node)->next = NULL;
        (*new_node)->prev = NULL;

        return 0;
    }

    return -1;
}


/*
list_insert():
-------------
Inserts and element in the list

lst : pointer to list structure
element : data element to be inserted in the list.

Returns :
-1 if memory allocation error
0 on success
*/
int list_insert(list_t *lst, void *element)
{
    NODEptr new_node;

    if ( __create_node( element, &new_node ) == 0 )
    {
        if ( lst->count == 0 )
        {
            lst->head = new_node;
        }
        else
        {
            NODEptr next = lst->head;
            lst->head = new_node;
            lst->head->next = next;
            next->prev = lst->head;
        }

        ++( lst->count );

        return 0;
    }

    return -1;
}


/*

isPresent()
-----------
Checks if there is an node which contains an element that matches a given element.

lst : pointer to list structure
ref_element : Data to be checked against all the list elements.
ret_node (return parameter) : if there is a match, *ret_node is assigned the pointer to the matched node

Returns:
NULL if no match
pointer to matching element
*/
void *isPresent(list_t *lst, void *ref_element, void **ret_node)
{
    NODEptr node = lst->head;

    int flag = 0;

    if ( !node )
    {
        if ( ret_node )
        {
            *(NODEptr *)ret_node = NULL;
        }

        return NULL;
    }

    while ( !flag && node )
    {
        if (lst->comp_keys(node->element, ref_element) == 0)
        {
            flag = 1;
        }
        else
        {
            node = node->next;
        }
    }

    if ( ret_node )
    {
        if ( flag )
        {
            *(NODEptr *)ret_node = ( void * )node;
        }
        else
        {
            *(NODEptr *)ret_node = NULL;
        }
    }

    return ((node)?node->element:NULL);
}


/*

list_delete()
--------------
Deletes a node from the list

lst : pointer to the list structure
node : pointer to the node to be deleted
*/
void list_delete(list_t *lst, void *node)
{
    NODEptr save = node;
    NODEptr prev, next;

    if (!node)
    {
        return;
    }

    if (save) {

        prev = save->prev;
        next = save->next;

        if (!prev && !next)
        {
            lst->head = NULL;
        }
        else if(!prev && next)
        {
            lst->head = next;
            next->prev = NULL;
        }
        else if(prev && !next)
        {
            prev->next = NULL;
        }
        else
        {
            prev->next = next;
            next->prev = prev;
        }

        n64_free( save->element );
        n64_free( save );

        --(lst->count);
    }
}


/*
list_cleanup()
-------------

Frees all nodes in the given list
*/
void list_cleanup(list_t *lst)
{
    NODEptr node, next;
    node = lst->head;

    while ( node )
    {
        next = node->next;

        n64_free(node->element);
        n64_free(node);

        node = next;
    }
}
