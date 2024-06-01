//
// Created by 谢子南 on 24-6-1.
//

#include <nar/heap.h>
#include <string.h>
#include "nar/mem.h"
#include "memory.h"
#include "nar/printk.h"
#include "nar/panic.h"

typedef char ALIGN[16];

union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    /* force the header to be aligned to 16 bytes */
    ALIGN stub;
};
typedef union header header_t;

static header_t *head = NULL, *tail = NULL;

static header_t *get_free_block(size_t size)
{
    header_t *curr = head;
    while(curr) {
        /* see if there's a free block that can accomodate requested size */
        if (curr->s.is_free && curr->s.size >= size)
            return curr;
        curr = curr->s.next;
    }
    return NULL;
}

static void free(void *block)
{
    header_t *header, *tmp;
    /* program break is the end of the process's data segment */
    void *programbreak;

    if (!block)
        return;
    // pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block - 1;
    /* kbrk(0) gives the current program break address */
    programbreak = kbrk(0);

    /*
       Check if the block to be freed is the last one in the
       linked list. If it is, then we could shrink the size of the
       heap and release memory to OS. Else, we will keep the block
       but mark it as free.
     */
    if ((char*)block + header->s.size == programbreak) {
        if (head == tail) {
            head = tail = NULL;
        } else {
            tmp = head;
            while (tmp) {
                if(tmp->s.next == tail) {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        /*
           kbrk() with a negative argument decrements the program break.
           So memory is released by the program to OS.
        */
        kbrk(0 - (int)header->s.size - (int)sizeof(header_t));
        /* Note: This lock does not really assure thread
           safety, because kbrk() itself is not really
           thread safe. Suppose there occurs a foregin kbrk(N)
           after we find the program break and before we decrement
           it, then we end up realeasing the memory obtained by
           the foreign kbrk().
        */
        // pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    // pthread_mutex_unlock(&global_malloc_lock);
}

static void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;
    if (!size)
        return NULL;
    // pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if (header) {
        /* Woah, found a free block to accomodate requested memory. */
        header->s.is_free = 0;
        // pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header + 1);
    }
    /* We need to get memory to fit in the requested block and header from OS. */
    total_size = sizeof(header_t) + size;
    block = kbrk(total_size);
    if (block == (void*) -1) {
        // pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if (!head)
        head = header;
    if (tail)
        tail->s.next = header;
    tail = header;
    // pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header + 1);
}

static void *calloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    if (!num || !nsize)
        return NULL;
    size = num * nsize;
    /* check mul overflow */
    if (nsize != size / num)
        return NULL;
    block = malloc(size);
    if (!block)
        return NULL;
    memset(block, 0, size);
    return block;
}

static void *realloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    if (!block || !size)
        return malloc(size);
    header = (header_t*)block - 1;
    if (header->s.size >= size)
        return block;
    ret = malloc(size);
    if (ret) {
        /* Relocate contents to the new bigger block */
        memcpy(ret, block, header->s.size);
        /* Free the old memory block */
        free(block);
    }
    return ret;
}

void* kalloc(size_t size)
{
    return malloc(size);
}

void kfree(void* ptr)
{
    free(ptr);
}

void heap_init()
{
    LOG("Heap Manager Started!\n");
}
