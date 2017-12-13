#ifndef __N64MEMORY_H
#define __N64MEMORY_H

#include <stdio.h>
#include <string.h>

extern void *malloc(size_t n);
extern void free(void *p);

typedef struct kv_s
{
    uint32_t address;
    size_t size;
} kv_t, *kv_p;

static kv_p kv_set = (kv_p)NULL; 

size_t num_kv = 0;
size_t last_size = 0;

// added for internal convenience
static int last_mapfind_index = -1;

#if 0
#define n64_free(b) \
free((b))

//n64_free2((b), __FILE__, __LINE__)

#define n64_malloc(s) \
malloc((s))

//n64_malloc2((s), __FILE__, __LINE__)
#endif

#define n64_memcpy(d,s,t) \
memcpy((d),(s),(t))

#define n64_memmove_naive_no_malloc(d,s,n) \
memmove((d),(s),(n))

#define n64_memmove_naive(d,s,n) \
memmove((d),(s),(n))

#define n64_memmove(d,s,n) \
memmove((d),(s),(n))

#define n64_memmove_old_with_potential_bug(d,s,n) \
memmove((d),(s),(n))

#define n64_memset(p,v,n) \
memset((p),(v),(n))

#define n64_memset2(p,v,n) \
memset((p),(v),(n))

#define n64_realloc(p,n) \
realloc((p),(n))


void n64_free2(void *buf, char *file, int line);
void *n64_malloc2(size_t size_to_alloc, char *file, int line);
void *n64_memcpy2(void *d, const void *s, size_t n);
void *n64_memmove_naive_no_malloc2(void *dest, const void *src, size_t n);
void *n64_memmove_naive2(void *dest, const void *src, size_t n);
void *n64_memmove2(void *d, const void *s, size_t n);
void *n64_memmove_old_with_potential_bug2(void *d, const void *s, size_t n);
void *n64_memset2_(void *p, int v, size_t n);
void *n64_memset22(void *p, int v, size_t n);
void *n64_realloc2(void *p, size_t n);

size_t get_allocated_byte_count(void);
size_t get_unmapped_free_count(void);
size_t get_null_free_count(void);

static void mapresize(size_t new_size);
void mapadd(uint32_t address, size_t size);
void mapremove(uint32_t address);
kv_t *mapfind(uint32_t address);
void mapdump(void);
void new_kv(size_t size);
size_t active_allocations(void);


size_t active_allocations(void)
{
    return num_kv;
}


void mapadd(uint32_t address, size_t size)
{
	if (NULL == kv_set)
	{
		new_kv(16);
	}
	
#ifdef KV_DEBUG // start #ifdef KV_DEBUG
    kv_t *kv = mapfind(address);
#if 1 // start #if 1
	if (NULL != kv)
	{
		printf("mapadd: already mapped kv[%d] == [0x%08X,0x%08X]\n", last_mapfind_index, kv->address, kv->size);
        exit(-1);
	}
#endif // end #if 1
#if 0 // start #if 0
/*    if (NULL != kv)
    {
        kv->address = address;
        kv->size = size;
    }*/
#endif // end #if 0
#if 1
/*    else
    {*/
#endif // end #if 1
#endif // end #ifdef KV_DEBUG
    if (num_kv == last_size)
    {
        mapresize(last_size * 2);
    }

    kv_set[num_kv].address = address;
    kv_set[num_kv].size = size;

    num_kv += 1;
#ifdef KV_DEBUG // start #ifdef KV_DEBUG
#if 1
/*    }*/
#endif
#endif
}


static void mapresize(size_t new_size)
{
    size_t i;

    kv_t *tmp_set = (kv_t *)malloc(new_size * sizeof(kv_t));
    memset(tmp_set, 0xFF, new_size * sizeof(kv_t));

    for (i=0;i<num_kv;i++)
    {
        tmp_set[i].address = kv_set[i].address;
        tmp_set[i].size = kv_set[i].size;
    }

    free(kv_set);

	kv_set = NULL;
    kv_set = tmp_set;

    last_size = new_size;
}


void new_kv(size_t size)
{
    kv_set = (kv_t*)malloc(size * sizeof(kv_t));
    memset(kv_set, 0xFF, size * sizeof(kv_t));

    num_kv = 0;
    last_size = size;
}


kv_p mapfind(uint32_t address)
{
    size_t i;

    for (i=0;i<num_kv;i++)
    {
        if(kv_set[i].address == address)
        {
			last_mapfind_index = i;
            return (kv_t *)&kv_set[i];
        }
    }

	last_mapfind_index = -1;
    return (kv_p)NULL;
}


void mapdump(void)
{
    size_t i;

    for (i=0;i<num_kv;i++)
    {
        printf("[%08X,%08X]\n",(unsigned int)kv_set[i].address,(unsigned int)kv_set[i].size);
    }
}


void mapremove(uint32_t address)
{
    size_t i;
    int found_index = -1;

    for (i=0;i<num_kv;i++)
    {
        if (kv_set[i].address == address)
        {
            found_index = i;
            goto do_remove;
        }
    }

    do_remove:
        if (found_index != -1)
        {
            num_kv -= 1;

            for (i=found_index;i<num_kv;i++)
            {
                kv_set[i].address = kv_set[i+1].address;
                kv_set[i].size = kv_set[i+1].size;
            }

            kv_set[num_kv].address = -1;
            kv_set[num_kv].size = -1;
        }
}

#endif //__N64MEMORY_H
