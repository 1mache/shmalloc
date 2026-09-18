#ifndef SHMALLOC_C
#define SHMALLOC_C

#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "types.h"

#define ENTER_KEY 10
#define ALLOC_KEY 65
#define FREE_KEY  66

#define DEBUG_MAGIC 777777

typedef struct MetaHeader
{
    u64 size;
    struct MetaHeader* prev;
    struct MetaHeader* next;
    int _debug;
    b8 free;
} MetaHeader;

#define META_SIZE sizeof(MetaHeader)

typedef struct MemBuffer
{
    byte* start;
    byte* end;
    u64   capacity;
} MemBuffer;

// head of the free list
static MetaHeader* free_list_head = NULL;
// last node in the free list
static MetaHeader* free_list_last = NULL;

void membuffer_init(MemBuffer* buffer, byte* resource, u64 capacity)
{
    buffer->start    = resource;
    buffer->end      = resource;
    buffer->capacity = capacity;
}

static void meta_header_init(MetaHeader* header, u64 size)
{
    header->size = size;
    header->next = NULL;
    header->prev = NULL;
    header->free = FALSE;
    header->_debug = DEBUG_MAGIC;
}

static MetaHeader* alloc_new_block(MemBuffer* buffer, u64 requested_bytes)
{
    u64 allocated_bytes = requested_bytes + META_SIZE;

    if((buffer->end - buffer->start) + allocated_bytes > buffer->capacity)
    {
        return NULL; // don't have space for allocation of this size 
    }

    // start of header = current buffer tail
    MetaHeader* returned = (MetaHeader*)buffer->end;
    meta_header_init(returned, requested_bytes);
    buffer->end += allocated_bytes;

    // update free list last
    if(!free_list_last)
    {
        free_list_head = free_list_last = returned;
    }
    else
    {
        returned->prev = free_list_last;
        free_list_last->next = returned;
        free_list_last = returned;
    }

    return returned;
}

static MetaHeader* find_free_block(u64 size)
{
    if(!free_list_head) 
    {
        return NULL;
    }

    MetaHeader* current = free_list_head;
    while(current)
    {
        if(current->free && current->size >= size)
        {
            current->free = FALSE;
            // identify it as allocated
             current->_debug = DEBUG_MAGIC;

            // TODO: update current header size if not null?
            return current;
        }
        current = current->next;
    }

    return current;
}

void* shmalloc_buffered(MemBuffer* buffer ,u64 requested_bytes)
{
    if(requested_bytes <= 0)
    {
        return NULL;
    }

    MetaHeader* ret_address;
    if(!free_list_head)
    {
        ret_address = alloc_new_block(buffer, requested_bytes);
        // function internally updated free list
    }
    else
    {
        ret_address = find_free_block(requested_bytes);
        if(!ret_address)
        {
            ret_address = alloc_new_block(buffer, requested_bytes);
        }
    }

    // move forward only if actually allocated block
    if(ret_address)
    {
        // move 1 header forward 
        ret_address += 1;
    }

    // return the address of the start of memory right after header
    return ret_address;
}

void free_buffered(MemBuffer* buffer ,void* ptr)
{
    if(!ptr) 
    {
        return;
    }
    if(!buffer->start || !buffer->end)
    {
        return; // invalid buffer
    } 
    if(buffer->start >= buffer->end) 
    {
        return; // empty buffer
    }
    
    // go back 1 header size from given ptr
    MetaHeader* freed_node = (MetaHeader*)((byte*)ptr - META_SIZE);
    assert(freed_node->_debug == DEBUG_MAGIC && "Free of corrupted ptr requested");
    freed_node->_debug = 0; // to catch freed nodes
    
    // if node has next node that is free we can merge their blocks' free space
    if(freed_node->next && freed_node->next->free) 
    {
        MetaHeader* merged = freed_node->next;
        merged->_debug = 0;
        // remove it from the list
        freed_node->next = merged->next;
        if(freed_node->next)
        {
            merged->next->prev = freed_node; 
        }
        // merge
        freed_node->size += META_SIZE + merged->size;  
    }

    // after merge this can be false even tho was true on previous if statement.
    // special case for last in list:
    if(!freed_node->next)
    {
        u64 allocated_bytes = freed_node->size + META_SIZE;
        assert(buffer->end - allocated_bytes >= buffer->start && "Something went wrong, request free of bigger size than we have");
        buffer->end -= allocated_bytes;
        // we deleted last so update tail
        free_list_last = freed_node->prev;
        
        // move free_list_last to last non free node 
        while(free_list_last && free_list_last->free)
        {
            free_list_last = free_list_last->prev;
        }
        
        //update buffer->end
        if(!free_list_last)
        {
            //special case when freed the only node
            buffer->end = buffer->start;
            free_list_head = NULL; //free list empty
        }
        else
        {
            //                                  move past header   move past block
            buffer->end = (byte*)(free_list_last) + META_SIZE + free_list_last->size; 
            free_list_last->next = NULL; // cut off free tailing nodes
        }
    }

    // mark as free
    freed_node->free = TRUE;
}

void free_all(MemBuffer* buffer)
{
    if(!buffer || buffer->end == buffer->start || buffer->capacity == 0)
    {
        // nothing to free
        return;
    }

    buffer->end = buffer->start;
    free_list_head = free_list_last = NULL;
}

#endif