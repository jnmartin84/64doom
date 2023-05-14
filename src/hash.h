#ifndef __HASH_H
#define __HASH_H

#include "dll.h"

// Copyright (C) 2012 Dennis Francis<dennisfrancis.in@gmail.com>

typedef struct HASHTABLE
{
    list_t *list_arr;
    int slots;
    unsigned int num_elements;
    int element_size;

    // params may be any generic data type containing parameters required
    // for calculation of hash value.
    unsigned long int (* hash)( void *element, void *params );

    void **element_list;
    void *hash_params; // Must be dynamically allocated
    char memFreed; // Y or N ; Before calling hashtable_init() set this to 'Y'
}
hashtable_t;

int hashtable_init( hashtable_t *ht, int slots,
int (* comp_str_keys )(void *, void *),
unsigned long int (* hash)( void *element, void *params ),
void *hash_params );

void hashtable_destroy( hashtable_t *ht );
int hashtable_insert( hashtable_t *ht, void *element, long int slot );

/*
Returns pointer to the element if found a match, else returns NULL
If ht_node != NULL,
*ht_node is assigned the address of the matching node
if no match is found, *ht_node will have NULL
*/
void *is_in_hashtable( hashtable_t *ht, void *ref_element, void **ht_node );

/*
Deletes the node pointed to by ht_node
*/
void hashtable_delete( hashtable_t *ht, unsigned int slot, void *ht_node );

// Returns an array of pointers to elements in the hashtable
// User should not free the array
void **get_elements_in_hashtable( hashtable_t *ht, int *num_elements, void *(* get_field )(void *), unsigned int (* get_index )(void *) );

#endif // __HASH_H
