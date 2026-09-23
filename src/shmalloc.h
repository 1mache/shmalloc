#ifndef SHMALLOC_H
#define SHMALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include "types.h"

typedef struct MetaHeader
{
    u64 size;
    struct MetaHeader* prev;
    struct MetaHeader* next;
    int _debug;
    b8 free;
} MetaHeader;

// alligned due to struct allignment
#define META_SIZE sizeof(MetaHeader)
#define PAGE_SIZE 4096 // assumed for simplicity

typedef struct MemArena
{
    byte* start;
    byte* end;
    u64   capacity;
    b8    owning;
} MemArena;

void  memarena_init(MemArena* arena, u64 capacity);
void  memarena_init_nonown(MemArena* arena, byte* resource, u64 capacity);
void  memarena_destroy(MemArena* arena);
void* arena_shmalloc(MemArena* arena ,u64 requested_bytes);
void  arena_free(MemArena* arena ,void* ptr);
void  memarena_free_all(MemArena* arena);

#endif //SHMALLOC_H 

#ifdef SHMALLOC_IMPLEMENTATION

#define MIN_ALLOC_SIZE 32
#define DEBUG_MAGIC 777777

#define ALIGN_UP(n,a) (((n) + ((a)-1)) & ~((a)-1))

// head of the free list
static MetaHeader* llist_head = NULL;
// last node in the free list
static MetaHeader* llist_last = NULL;

void memarena_init(MemArena* arena, u64 capacity)
{
    assert(sysconf(_SC_PAGE_SIZE) == PAGE_SIZE && "Weird system bro... page size not 4K");

    u64 arena_size = ALIGN_UP(capacity, PAGE_SIZE);

    void* ptr = mmap(NULL, arena_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(ptr == (void*)-1)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    arena->start    = ptr;
    arena->end      = ptr;
    arena->capacity = arena_size;
    arena->owning   = TRUE;
}

// non owning version (you manage the resource)
void memarena_init_nonown(MemArena* arena, byte* resource, u64 capacity)
{
    arena->start    = resource;
    arena->end      = resource;
    arena->capacity = capacity;
    arena->owning   = FALSE;
}

// not necessary to call when non owning
void memarena_destroy(MemArena* arena)
{
    if(arena->owning)
    {
        int retcode = munmap(arena->start, arena->capacity);
        if(retcode == -1)
        {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
    }
}

static void meta_header_init(MetaHeader* header, u64 size)
{
    assert(size > 0 && "meta_header_init: size <= 0");

    header->size = size;
    header->next = NULL;
    header->prev = NULL;
    header->free = FALSE;
    header->_debug = DEBUG_MAGIC;
}

static MetaHeader* alloc_new_block(MemArena* arena, u64 size)
{
    assert(size > 0 && "alloc_new_block: size <= 0");

    u64 allocated_bytes = size + META_SIZE;

    if((arena->end - arena->start) + allocated_bytes > arena->capacity)
    {
        return NULL; // don't have space for allocation of this size 
    }

    // start of header = current arena tail
    MetaHeader* returned = (MetaHeader*)arena->end;
    meta_header_init(returned, size);
    arena->end += allocated_bytes;

    // update free list last
    if(!llist_last)
    {
        llist_head = llist_last = returned;
    }
    else
    {
        returned->prev = llist_last;
        llist_last->next = returned;
        llist_last = returned;
    }

    return returned;
}

// wipes everything in between. doesnt update freelist. callers responsiblity.
static MetaHeader* merge_blocks(MetaHeader* header0, MetaHeader* header1)
{
    // asserts because internal function, should crash if used wrong
    assert(header0 && "merge_blocks recieved NULL header0");
    assert(header1 && "merge_blocks recieved NULL header1");
    assert(header0 != header1 && "merge_blocks recieved same header");
    
    MetaHeader* to     = MIN(header0, header1);
    MetaHeader* merged = MAX(header0, header1);

    merged->_debug = 0;
    // remove it from the list
    to->next = merged->next;
    if(to->next)
    {
        to->next->prev = to; 
    }
    // merge
    // size = all space in between headers + merged header size and merged size 
    to->size = ((byte*)merged - (byte*)(to+1)) + (META_SIZE + merged->size);  

    return to;
}

static MetaHeader* find_free_block(u64 size)
{
    assert(size > 0 && "find_free_block: size <= 0");

    if(!llist_head) 
    {
        return NULL;
    }

    MetaHeader* current = llist_head;
    while(current)
    {
        if(current->free && current->size >= size)
        {
            current->free = FALSE;
            // identify it as allocated
            current->_debug = DEBUG_MAGIC;

            // can split?
            if((current->size - size) > MIN_ALLOC_SIZE + META_SIZE)
            {
                // node that holds rest of space
                MetaHeader* rest = (MetaHeader*)((byte*)current + META_SIZE + current->size);
                meta_header_init(rest, current->size - size);
                
                // add new node to list
                rest->prev = current;
                if(current->next)
                {
                    current->next->prev = rest;
                }
                current->next = rest;

                // update size
                current->size = size;
            }

            break; // found
        }
        // can merge with next in list?
        else if(current->free)
        {
            u64 total_free_space = current->size;
            MetaHeader* it = current;
            while(it->next && it->next->free && total_free_space < size)
            {
                total_free_space += META_SIZE + it->next->size;
                it = it->next;
            }
            assert(it && "find_free_block: merge iterator is NULL");
            
            if(total_free_space >= size)
            {
                merge_blocks(current, it);
                break; // found
            }
        }

        current = current->next;
    }

    return current;
}

void* arena_shmalloc(MemArena* arena, u64 requested_bytes)
{
    if(requested_bytes <= 0)
    {
        return NULL;
    }
    
    // align to WORD size
    u64 size = ALIGN_UP(requested_bytes, sizeof(void*));
    MetaHeader* ret_address = NULL;
    if(!llist_head)
    {
        ret_address = alloc_new_block(arena, size);
        // function internally updated free list
    }
    else
    {
        ret_address = find_free_block(size);
        if(!ret_address)
        {
            ret_address = alloc_new_block(arena, size);
        }
    }
    // couldn't allocate
    if(!ret_address)
    {
        return NULL;
    }
    
    // move ptr after the header 
    return ret_address + 1;
}

void arena_free(MemArena* arena ,void* ptr)
{
    if(!ptr) 
    {
        return;
    }
    if(!arena->start || !arena->end)
    {
        return; // invalid arena
    } 
    if(arena->start >= arena->end) 
    {
        return; // empty arena
    }
    
    // go back 1 header size from given ptr
    MetaHeader* freed_node = (MetaHeader*)((byte*)ptr - META_SIZE);
    assert(freed_node->_debug == DEBUG_MAGIC && "Free of corrupted ptr requested");
    freed_node->_debug = 0; // to catch freed nodes

    // if prev block is free, merge them
    if(freed_node->prev && freed_node->prev->free)
    {
        // if this is the tail
        if(freed_node == llist_last) 
        {
            llist_last = freed_node->prev;
        }
        freed_node = merge_blocks(freed_node, freed_node->prev);
    }
    
    // if next block is free, merge them.
    if(freed_node->next && freed_node->next->free)
    {
        merge_blocks(freed_node, freed_node->next);
    }

    // special case for last in list:
    if(!freed_node->next)
    {
        u64 allocated_bytes = freed_node->size + META_SIZE;
        assert(arena->end - allocated_bytes >= arena->start && "Something went wrong, request free of bigger size than we have");
        arena->end -= allocated_bytes;
        // we deleted last so update tail
        llist_last = freed_node->prev;
        
        // move llist_last to last non free node 
        while(llist_last && llist_last->free)
        {
            llist_last = llist_last->prev;
        }
        
        //update arena->end
        if(!llist_last)
        {
            //special case when freed the only node
            arena->end = arena->start;
            llist_head = NULL; //free list empty
        }
        else
        {
            //                                  move past header   move past block
            arena->end = (byte*)(llist_last) + META_SIZE + llist_last->size; 
            llist_last->next = NULL; // cut off free tailing nodes
        }
    }

    // mark as free
    freed_node->free = TRUE;
}

void memarena_free_all(MemArena* arena)
{
    if(!arena || arena->end == arena->start || arena->capacity == 0)
    {
        // nothing to free
        return;
    }

    arena->end = arena->start;
    llist_head = llist_last = NULL;
}

#endif // SHMALLOC_IMPLEMENTATION