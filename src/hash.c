// Copyright (C) 2012 Dennis Francis<dennisfrancis.in@gmail.com>

#include <libdragon.h>
#include <stdlib.h>

#include "hash.h"
#include <string.h>

#define n64_free(x) free((x))
#define n64_malloc(x) malloc((x))


/*

hashtable_init()
----------------
Initialize the hashtable structure instance.

ht : address of the hashtable to be initialized
slots : number of slots to be created in the hashtable

comp_keys : Pointer to a function that returns 0 when the keys of the arguments are same. otherwise some non-zero number.

hash : Pointer to hash function which takes the element structure and some arbitary hash parameter structure as arguments.

hash_params : Pointer to a data structure holding parameter details of the hash function.

Return value
------------
-1 if Unsuccessful in allocating space for the slots.
0 if no error

*/
int hashtable_init(hashtable_t *ht, int slots, int (* comp_keys )(void *, void *), unsigned long int (* hash)( void *element, void *params ), void *hash_params)
{
    int i;

    if (ht->memFreed == 'N')
    {
        hashtable_destroy( ht );
    }

    ht->memFreed = 'Y';
    ht->slots = slots;
    ht->hash = hash;
    ht->hash_params = hash_params;
    ht->list_arr = ( list_t * )n64_malloc( slots*sizeof(list_t) );

    if (!( ht->list_arr))
    {
        return -1;
    }

    for (i=0; i<slots; i++)
    {
        list_init( ht->list_arr + i, comp_keys );
    }

    ht->num_elements = 0;
    ht->element_list = 0;

    ht->memFreed = 'N';

    return 0;
}


/*

hashtable_destroy()
-------------------
Cleans up the dynamically allocated memory regarding the hashtable and chains.

ht : pointer to hashtable that is to be destroyed
*/
void hashtable_destroy(hashtable_t *ht)
{
    int i;

    if (ht->memFreed == 'Y')
    {
        return;
    }

    for (i=0; i<ht->slots; i++)
    {
        list_cleanup( ht->list_arr + i ); // Free each list
    }

    n64_free( ht->list_arr ); // Free the array of list headers
    n64_free( ht->element_list ); // Null check is done by free()

    //free( ht->hash_params ); // do only if dynamically allocated

    ht->memFreed = 'Y';
}


/*

hashtable_insert() :
--------------------
Inserts a new element into the hashtable. Does not check if the element
already exists.
ht : pointer to hashtable
element : pointer to the data item to be stored
slot : provide slot number if hash is already calculated, else give a negative number

Returns :
--------
-1 if error allocating memory for new node.
0 on success

*/
int hashtable_insert( hashtable_t *ht, void *element, long int slot )
{
    // Does not check for existence
    int target_slot = (slot < 0)?ht->hash( element, ht->hash_params ): slot ;
    int ret_val = list_insert( ht->list_arr + target_slot, element );

    if( ret_val == 0 )
    {
        ++(ht->num_elements);
    }

    return ret_val;
}


/*
is_in_hashtable()
-----------------

Check if an element is present in the hashtable or not

ht : pointer to hashtable
ref_element : Element to be searched in the hashtable for existence
ht_node(return parameter) : pointer to node where the matching element is found

Returns :
NULL if no matching element is found
pointer to matching element if found

*/
void *is_in_hashtable(hashtable_t *ht, void *ref_element, void **ht_node)
{
    int target_slot = ht->hash( ref_element, ht->hash_params );

    return isPresent( ht->list_arr + target_slot, ref_element, ht_node );
}


/*
hashtable_delete()
------------------

Deletes a node corresponding to an element of known slot

ht : pointer to hashtable
slot : slot where the element resides
ht_node : pointer to the linked list node which holds the element.

*/
void hashtable_delete(hashtable_t *ht, unsigned int slot, void *ht_node)
{
    list_delete( ht->list_arr + slot, ht_node );

    --(ht->num_elements);
}


/*
get_elements_in_hashtable()
---------------------------

Returns a pointer to an array of pointers. Each pointer in that array will point to some field in the data elements.
The user must not free this pointer. It will be cleaned up during
hashtable_destroy()

ht : pointer to hashtable
num_elements : number of elements placed in the returned array
get_field : a function which extracts some required field from the element. If this is NULL
the function will return an array of pointers, each pointing to the elements itself.
get_index : a function which extracts the index where the element must be stored. If this is null, the
the index where an element is stored is dependant on the order in which nodes are traversed.

*/
void **get_elements_in_hashtable(hashtable_t *ht, int *num_elements, void *(* get_field )(void *), unsigned int(* get_index )(void *))
{
    int i;
    int default_index;
    int index = 0;

    list_t *lst;
    NODEptr node;

    if (ht->element_list)
    {
        n64_free( ht->element_list );
    }

    ht->element_list = (void **)n64_malloc( (ht->num_elements)*sizeof(void *) );

    if (!(ht->element_list))
    {
        return 0;
    }

    for (i = 0, default_index = 0; i<ht->slots; i++)
    {
        lst = ht->list_arr+i;

        if( lst->count )
        {
            node = lst->head;

            while( node )
            {
                index = ( get_index )?get_index( node->element ) : default_index++;
                ((char **)ht->element_list)[ index ] = (get_field)? get_field( node->element ) : node->element;
                node = node->next;
            }
        }
    }

    if (num_elements)
    {
        *num_elements = ht->num_elements;
    }

    return ht->element_list;
}
