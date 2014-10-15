#ifndef __N64MEMORY_H
#define __N64MEMORY_H

#include <libdragon.h>
#include <stdlib.h>

typedef struct kv_s
{
    uint32_t address;
    uint32_t size;
} kv_t;

int num_kv = 0;
int last_size = 0;

void n64_free(void *buf);
void *n64_malloc(size_t size_to_alloc);
void *n64_memcpy(void *d, const void *s, size_t n);
void *n64_memmove(void *d, const void *s, size_t n);
void *n64_memset(void *p, int v, size_t n);
void *n64_memset2(void *p, int v, size_t n);

int get_allocated_byte_count();
int get_unmapped_free_count();

static kv_t *mapresize(kv_t *set, int new_size);
void mapadd(kv_t *set, uint32_t address, int size);
void mapremove(kv_t *set, uint32_t address);
kv_t *mapfind(kv_t *set, uint32_t address);
kv_t *new_kv(int size);
int active_allocations();


int active_allocations()
{
    return num_kv;
}


void mapadd(kv_t *set, uint32_t address, int size)
{
    kv_t *kv = mapfind(set, address);

    if (kv != 0)
    {
        kv->address = address;
        kv->size = size;
    }
    else
    {
        if (num_kv - 1 >= last_size)
        {
            set = mapresize(set, last_size * 2);
        }

        set[num_kv].address = address;
        set[num_kv].size = size;

        num_kv += 1;
    }
}


static kv_t *mapresize(kv_t *set, int new_size)
{
    int i;

    kv_t *new_set = (kv_t *)malloc(new_size * sizeof(kv_t));

    for (i=0;i<new_size;i++)
    {
        new_set[i].address = set[i].address;
        new_set[i].size = set[i].size;
    }

    last_size = new_size;

    free(set);

    return new_set;
}


kv_t *new_kv(int size)
{
    int i;

    kv_t *new_set = (kv_t*)malloc(size * sizeof(kv_t));

    for (i=0;i<size;i++)
    {
        new_set[i].address = -1;
        new_set[i].size = -1;
    }

    num_kv = 0;
    last_size = size;

    return new_set;
}


kv_t *mapfind(kv_t *set, uint32_t address)
{
    int i;

    for (i=0;i<num_kv;i++)
    {
        if(set[i].address == address)
        {
            return (kv_t *)&set[i];
        }
    }

    return (kv_t *)0;
}


void mapremove(kv_t *set, uint32_t address)
{
    int i;
    int found_index = -1;

    for (i=0;i<num_kv;i++)
    {
        if (set[i].address == address)
        {
            found_index = i;
            goto do_remove;
        }
    }

    do_remove:
    if (found_index != -1)
    {
        for (i=found_index;i<num_kv - 1;i++)
        {
            set[i].address = set[i+1].address;
            set[i].size = set[i+1].size;
        }

        set[num_kv-1].address = -1;
        set[num_kv-1].size = -1;

        num_kv -= 1;
    }
}

#endif //__N64MEMORY_H
