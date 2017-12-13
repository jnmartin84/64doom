#include <libdragon.h>
#include <stddef.h> /* for size_t */
#include <stdio.h>
#include <stdlib.h> /* for memcpy */

#include "n64memory.h"

char memory_log[256];

#define ALIGNED(addr) (((int)addr % 4) == 0)

static void mapresize(size_t new_size);

void mapadd(uint32_t address, size_t size);
void mapremove(uint32_t address);
void mapdump(void);
kv_t *mapfind(uint32_t address);
void new_kv(size_t size);
size_t active_allocations(void);

size_t allocated_bytes = 0;
size_t null_frees = 0;
size_t unmapped_frees = 0;


size_t get_allocated_byte_count()
{
    return allocated_bytes;
}


size_t get_unmapped_free_count()
{
    return unmapped_frees;
}


size_t get_null_free_count()
{
    return null_frees;
}

/*
NAME

    realloc - memory reallocator

SYNOPSIS

    #include <stdlib.h>

    void *realloc(void *ptr, size_t size);

DESCRIPTION

    [CX] [Option Start] The functionality described on this reference page is aligned with the ISO C standard. 
    Any conflict between the requirements described here and the ISO C standard is unintentional. 
    This volume of IEEE Std 1003.1-2001 defers to the ISO C standard. [Option End]

    The realloc() function shall change the size of the memory object pointed to by ptr to the size specified by size. 
    The contents of the object shall remain unchanged up to the lesser of the new and old sizes. If the new size of the 
    memory object would require movement of the object, the space for the previous instantiation of the object is freed. 
    If the new size is larger, the contents of the newly allocated portion of the object are unspecified. If size is 0 and 
    ptr is not a null pointer, the object pointed to is freed. If the space cannot be allocated, the object shall remain unchanged.

    If ptr is a null pointer, realloc() shall be equivalent to malloc() for the specified size.

    If ptr does not match a pointer returned earlier by calloc(), malloc(), or realloc() or if the space has previously been 
    deallocated by a call to free() or realloc(), the behavior is undefined.

    The order and contiguity of storage allocated by successive calls to realloc() is unspecified. The pointer returned if the 
    allocation succeeds shall be suitably aligned so that it may be assigned to a pointer to any type of object and then 
    used to access such an object in the space allocated (until the space is explicitly freed or reallocated). Each such 
    allocation shall yield a pointer to an object disjoint from any other object. The pointer returned shall point to the start 
    (lowest byte address) of the allocated space. If the space cannot be allocated, a null pointer shall be returned.

RETURN VALUE

    Upon successful completion with a size not equal to 0, realloc() shall return a pointer to the (possibly moved) allocated space. 
    If size is 0, either a null pointer or a unique pointer that can be successfully passed to free() shall be returned. 
    If there is not enough available memory, realloc() shall return a null pointer [CX] [Option Start]  and set errno to [ENOMEM]. [Option End]

ERRORS

    The realloc() function shall fail if:

    [ENOMEM]
        [CX] [Option Start] Insufficient memory is available. [Option End]
*/
void *n64_realloc2(void *ptr, size_t size)
{
    void *return_buffer;

//    printf("args-->[ptr:size]<->[%p,%08X]\n",ptr,(unsigned int)size);

    // "If size is 0"
    if (0 == size)
    {
//        printf("0 == size\n");
        // "and ptr is not a null pointer, the object pointed to is freed."
        if (NULL != ptr)
        {
//            printf("\t0 != ptr\n");
            n64_free(ptr);
        }

        // "If size is 0, [...] a null pointer [...] shall be returned."
        return_buffer = 0;
        goto realloc_return;
    }

    // "If ptr is a null pointer, realloc() shall be equivalent
    //  to malloc() for the specified size."
    if (NULL == ptr)
    {
//        printf("0 == ptr\n");
        return_buffer = n64_malloc(size);
        goto realloc_return;
    }

    return_buffer = n64_malloc(size);

    // "If there is not enough available memory"
    if (NULL == return_buffer)
    {
//        printf("0 == return_buffer\n");
        // "realloc() shall return a null pointer"
        return_buffer = 0;
        // "and set errno to [ENOMEM]."
//        __errno = ENOMEM;
        goto realloc_return;
    }

    // retrieve size of old buffer
//    printf("%08X\n",(unsigned int)( ((uint32_t)ptr)&0x0FFFFFFF));
    kv_t *old_buffer = mapfind((uint32_t)ptr);

    if (NULL == old_buffer)
    {
//        printf("0 == old_buffer\n");
//        mapdump(kv_set);
        return_buffer = n64_malloc(size);
        goto realloc_return;
    }

//    printf("old_buffer-->[ptr:size]<->[%08X,%08X]\n", (unsigned int)old_buffer->address, (unsigned int)old_buffer->size);

    size_t old_size = old_buffer->size;
    size_t copy_size = size;

    // "The contents of the object shall remain unchanged up to the lesser of the new and old sizes."
    if (old_size < size)
    {
        copy_size = old_size;
    }

    // "If the new size is larger, the contents of the newly allocated portion of the object are unspecified."
    n64_memcpy(return_buffer, ptr, copy_size);

    // "If the new size of the memory object would require movement of the object,
    // the space for the previous instantiation of the object is freed."
    n64_free(ptr);

    // single point of return
    realloc_return:
        return return_buffer;
}


void *n64_malloc2(size_t size_to_alloc, char *file, int line)
{
    n64_memset(memory_log, 0x00, 256);
    sprintf(memory_log, "n64_malloc(0x%08X): %s %d\n", size_to_alloc, file, line);

    size_t adjusted_size = (size_to_alloc + 3) & ~3;
/*
    if (size_to_alloc < 8)
    {
        adjusted_size = 8;
    }

    else if (size_to_alloc % 8 != 0)
    {
        adjusted_size += (8 - (size_to_alloc % 8));
    }
*/
    void *buf = malloc(adjusted_size);

    if (NULL == buf)
    {
//        return buf;
        goto malloc_return;
    }

//    if (0 != buf)
//    {
    allocated_bytes += adjusted_size;
//    }

    // map address to size and store somewhere
    mapadd((uint32_t)buf, adjusted_size);

    malloc_return:
        return buf;
}


void n64_free2(void *buf, char *file, int line)
{
    n64_memset(memory_log, 0x00, 256);
    sprintf(memory_log, "n64_free(%p): %s %d\n", buf, file, line);

    if (NULL == buf)
    {
        null_frees += 1;
	    return;
    }

    kv_t *kv = mapfind((uint32_t)buf);

    // it got malloc'd but we missed it
    if (NULL == kv)
    {
	    unmapped_frees += 1;
#if 0
        free(buf);
#endif
		}
    // we saw the malloc too
    else
    {
        size_t size_to_free = kv->size;

	    mapremove((uint32_t)buf);

        free(buf);

        allocated_bytes -= size_to_free;
    }
}


// A sample naive, literal implementation:
void *n64_memmove_naive_no_malloc2(void *dest, const void *src, size_t n)
{
    unsigned char tmp[n];
    n64_memcpy(tmp,src,n);
    n64_memcpy(dest,tmp,n);
    return dest;
}


// A sample naive, literal implementation:
void *n64_memmove_naive2(void *dest, const void *src, size_t n)
{
    void *tmp = (void *)n64_malloc(n);
    if(NULL == tmp)
    {
        return dest;
    }
    n64_memcpy(tmp,src,n);
    n64_memcpy(dest,tmp,n);
    n64_free(tmp);
    return dest;
}


void *n64_memmove2(void *dest, const void *src, size_t size)
{
    int       i;
    int       size_in_uint32_ts;
    int       size_in_bytes;

    uint32_t *wd;
    uint32_t *ws;
    uint8_t  *bd;
    uint8_t  *bs;
    uint32_t *wtmp;
    uint8_t  *btmp;
    uint8_t  *d;
    uint8_t  *s;

    void *tmp = (void *)n64_malloc(size);

    if(NULL == tmp)
    {
//        return dest;
        goto memmove_return;
    }

    if (ALIGNED(dest) && ALIGNED(src))
    {
        wd = (uint32_t*)dest;
        ws = (uint32_t*)src;

        size_in_uint32_ts = size >> 2;
        size_in_bytes = size % 4;

        bd = (uint8_t*)(dest + (size_in_uint32_ts << 2));
        bs = (uint8_t*)(src + (size_in_uint32_ts << 2));

        wtmp = (uint32_t*)tmp;

        for (i=0;i<size_in_uint32_ts;i++)
        {
            wtmp[i] = ws[i];
        }

        for (i=0;i<size_in_uint32_ts;i++)
        {
            wd[i] = wtmp[i];
        }

        btmp = (uint8_t*)(tmp + (size_in_uint32_ts << 2));

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
        d = (uint8_t*)dest;
        s = (uint8_t*)src;

        btmp = (uint8_t*)tmp;

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

    memmove_return:
        return dest;
}


void *n64_memmove_old_with_potential_bug2(void *dest, const void *src, size_t size)
{
    int i;
    void *tmp = (void *)n64_malloc(size);

    if(NULL == tmp)
    {
        return dest;
    }

    if (ALIGNED(dest) && ALIGNED(src))
    {
        uint32_t *wd = (uint32_t*)dest;
        uint32_t *ws = (uint32_t*)src;
        uint8_t *bd = (uint8_t*)dest;
        uint8_t *bs = (uint8_t*)src;

        uint32_t *wtmp;
        uint8_t *btmp;

        int size_in_uint32_ts = size >> 2;
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


void *n64_memcpy2(void *dst, const void *src, size_t size)
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
void *n64_memset2_(void *ptr, int value, size_t num)
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
void *n64_memset22(void *ptr, int value, size_t num)
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
