#ifndef __DLL_H
#define __DLL_H
// Copyright (C) 2012 Dennis Francis<dennisfrancis.in@gmail.com>

typedef struct ND {

    struct ND *prev;
    struct ND *next;
    void *element;
    
} NODE, *NODEptr;


typedef struct {

    NODEptr head;
    int count;
    int (* comp_keys )(void *, void *);
    
} list_t;


void list_init( list_t *lst, int (* comp_keys)(void *, void *) );
int list_insert( list_t *lst, void *element );
void *isPresent( list_t *lst, void *ref_element, void **ret_node );

void list_delete( list_t *lst, void *node );
void list_cleanup( list_t *lst );
#endif // __DLL_H
