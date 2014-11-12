#include <libdragon.h>
#include <stdio.h>
#include <stdlib.h>

#include "n64memory.h"

#define ALIGNED(addr) (((int)addr % 4) == 0)

static kv_t *mapresize(kv_t *set, int new_size);

void mapadd(kv_t *set, uint32_t address, int size);
void mapremove(kv_t *set, uint32_t address);
kv_t *mapfind(kv_t *set, uint32_t address);
kv_t *new_kv(int size);
int active_allocations();

kv_t *kv_set; // array of dynamically allocated kv_t structs

int allocated_bytes = 0;
int null_frees = 0;
int unmapped_frees = 0;


int get_allocated_byte_count()
{
    return allocated_bytes;
}


int get_unmapped_free_count()
{
    return unmapped_frees;
}


int get_null_free_count()
{
    return null_frees;
}


void *n64_malloc(size_t size_to_alloc)
{
    if (0 == kv_set)
    {
        kv_set = new_kv(1024);
    }

    int adjusted_size = size_to_alloc;

    if (size_to_alloc < 8)
    {
        adjusted_size = 8;
    }

    else if (size_to_alloc % 8 != 0)
    {
        adjusted_size += (8 - (size_to_alloc % 8));
    }

    void *buf = malloc(adjusted_size);

    if (0 != buf)
    {
        allocated_bytes += adjusted_size;
    }

    // map address to size and store somewhere
    mapadd(kv_set, ((uint32_t)&buf)&0x0FFFFFFF, adjusted_size);

    return buf;
}


void n64_free(void *buf)
{
    if (NULL == buf)
    {
        null_frees += 1;
	return;
    }

    kv_t *kv = mapfind(kv_set, ((uint32_t)&buf)&0x0FFFFFFF);

    // it got malloc'd but we missed it
    if (0 == kv)
    {
	unmapped_frees += 1;
        free(buf);
    }
    // we saw the malloc too
    else
    {
        int size_to_free = kv->size;

	mapremove(kv_set, ((uint32_t)&buf)&0x0FFFFFFF);

        free(buf);

        allocated_bytes -= size_to_free;
    }
}


/**
RETURN VALUE

    The memmove() function shall return s1; no return value is reserved to indicate an error.

ERRORS

    No errors are defined.

NOTES FROM JNMARTIN84
    My code attempts to create a temporary buffer in order to meet the required semantics of memmove
    which include being able to copy overlapping buffers in a way that is indistinguishable from using
    a temporary buffer.
    In the event of an error creating a temp buffer to meet the required copy semantics,
    the n64_memmove function will return null.
*/
void *n64_memmove(void *dest, const void *src, size_t size)
{
    int i;
    void *tmp = (void *)n64_malloc(size);

    // no space to re-allocate, return null
    if (0 == tmp)
    {
        return 0;
    }

    if (ALIGNED(dest) && ALIGNED(src))
    {
        uint32_t *wd = (uint32_t*)dest;
        uint32_t *ws = (uint32_t*)src;
        uint8_t *bd = (uint8_t*)dest;
        uint8_t *bs = (uint8_t*)src;

        uint32_t *wtmp;
        uint8_t *btmp;

        int size_in_uint32_ts = size / 4;
        int size_in_bytes = size % 4;

        wtmp = (uint32_t*)tmp;

        for (i=0;i<size_in_uint32_ts;i++)
        {
            wtmp[i] = ws[i];
        }

        for (i=0;i<size_in_uint32_ts;i++)
        {
            wd[i] = wtmp[i];
        }

        btmp = (uint8_t*)tmp;

        for (i=0;i<size_in_bytes;i++)
        {
            btmp[i] = bs[i];
        }

        for (i=0;i<size_in_bytes;i++)
        {
            bd[i] = btmp[i];
        }
    }
    else
    {
        uint8_t *d = (uint8_t*)dest;
        uint8_t *s = (uint8_t*)src;

        uint8_t *btmp = (uint8_t*)tmp;

        for (i=0;i<size;i++)
        {
            btmp[i] = s[i];
        }

        for (i=0;i<size;i++)
        {
            d[i] = btmp[i];
        }
    }

    n64_free(tmp);

    return dest;
}


void *n64_memcpy(void *dst, const void *src, size_t size)
{
    uint8_t *bdst = (uint8_t *)dst;
    uint8_t *bsrc = (uint8_t *)src;
    uint32_t *wdst = (uint32_t *)dst;
    uint32_t *wsrc = (uint32_t *)src;

    int size_to_copy = size;

    if (ALIGNED(bdst) && ALIGNED(bsrc))
    {
        int words_to_copy = size_to_copy / 4;
        int bytes_to_copy = size_to_copy % 4;

        while (words_to_copy--)
        {
            *wdst++ = *wsrc++;
        }

        bdst = (uint8_t *)wdst;
        bsrc = (uint8_t *)wsrc;

        while (bytes_to_copy--)
        {
            *bdst++ = *bsrc++;
        }
    }
    else
    {
        int w_to_copy = size_to_copy / 4;
        int b_to_copy = size_to_copy % 4;

        while (w_to_copy > 0)
        {
            *bdst++ = *bsrc++;
            *bdst++ = *bsrc++;
            *bdst++ = *bsrc++;
            *bdst++ = *bsrc++;

            w_to_copy--;
        }

        while(b_to_copy--)
        {
            *bdst++ = *bsrc++;
        }
    }

    return dst;
}


// n64_memset is special-cased to fill zeros
// all but two or three calls in Doom had 0 as the fill value
void *n64_memset(void *ptr, int value, size_t num)
{
    uint32_t *w = (uint32_t *)ptr;
    uint8_t  *p = (uint8_t *)ptr;

    if (ALIGNED(ptr))
    {
        int words = num / 4;
        int bytes = num % 4;

        while (words--)
        {
            *w++ = 0x00000000;
        }

        p = (unsigned char*)w;

        while (bytes--)
        {
            *p++ = 0x00;
        }
    }
    else
    {
        while (num--)
        {
            *p++ = 0x00;
        }
    }

    return ptr;
}


// the non-special case version that accepts arbitrary fill value
void *n64_memset2(void *ptr, int value, size_t num)
{
    uint32_t	*w = (uint32_t *)ptr;
    uint8_t	*p = (uint8_t*)ptr;
    uint8_t	v;
    uint32_t	wv;

    v = (uint8_t)(value & 0x0000FF);
    wv = (v << 24) | (v << 16) | (v << 8) | v;

    if (ALIGNED(ptr))
    {
        int words = num / 4;
        int bytes = num % 4;
        while (words--)
        {
            *w++ = wv;
        }

        p = (unsigned char*)w;

        while (bytes--)
        {
            *p++ = v;
        }
    }
    else
    {
        while (num--)
        {
            *p++ = v;
        }
    }

    return ptr;
}
