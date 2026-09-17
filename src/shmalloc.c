#ifndef SHMALLOC_C
#define SHMALLOC_C

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef int8_t   i8 ;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8 ;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t  byte;   

typedef int8_t   b8;
#define TRUE  1
#define FALSE 0

#define Kb(n) ((u64)(n) << 10)
#define Mb(n) ((u64)(n) << 20)
#define Gb(n) ((u64)(n) << 30)

#define MAX(a,b) (((a) > (b)) ? (a):(b))
#define MIN(a,b) (((a) < (b)) ? (a):(b))

#define ENTER_KEY 10
#define ALLOC_KEY 65
#define FREE_KEY  66

#define DEBUG_MAGIC 777777

typedef struct AllocHeader
{
    u64 size;
    struct AllocHeader* prev;
    struct AllocHeader* next;
    int _debug;
    b8 free;
} AllocHeader;

#define META_SIZE sizeof(AllocHeader)

typedef struct MemDummyBuffer
{
    byte* start;
    byte* end;
    u64   capacity;
} MemDummyBuffer;

// head of the free list
static AllocHeader* free_list_head = NULL;
// last node in the free list
static AllocHeader* free_list_last = NULL;

static void init_header(AllocHeader* header, u64 size)
{
    header->size = size;
    header->next = NULL;
    header->prev = NULL;
    header->free = FALSE;
    header->_debug = DEBUG_MAGIC;
}

void init_alloc_buffer(MemDummyBuffer* buffer, byte* resource, u64 capacity)
{
    buffer->start    = resource;
    buffer->end      = resource;
    buffer->capacity = capacity;
}

static AllocHeader* alloc_new_block(MemDummyBuffer* buffer, u64 requested_bytes)
{
    u64 allocated_bytes = requested_bytes + META_SIZE;

    if((buffer->end - buffer->start) + allocated_bytes > buffer->capacity)
    {
        return NULL; // don't have space for allocation of this size 
    }

    // start of header = current buffer tail
    AllocHeader* returned = (AllocHeader*)buffer->end;
    init_header(returned, requested_bytes);
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

static AllocHeader* find_free_block(u64 size)
{
    if(!free_list_head) 
    {
        return NULL;
    }

    AllocHeader* current = free_list_head;
    while(current)
    {
        if(current->free && current->size >= size)
        {
            current->free = FALSE;
            // TODO: update current header size if not null?
            return current;
        }
        current = current->next;
    }

    return current;
}

void* dumb_allocate(MemDummyBuffer* buffer ,u64 requested_bytes)
{
    // TODO: validate size
    AllocHeader* ret_address;
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

void dumb_free(MemDummyBuffer* buffer ,void* ptr)
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
    AllocHeader* freed_node = (AllocHeader*)((byte*)ptr - META_SIZE);
    assert(freed_node->_debug == DEBUG_MAGIC && "Free of corrupted ptr requested");
    
    // special case for last in list:
    if(!freed_node->next)
    {
        u64 allocated_bytes = freed_node->size + META_SIZE;
        assert(buffer->end - allocated_bytes >= buffer->start && "Something went wrong, request free of bigger size than we have");
        buffer->end -= allocated_bytes;
        // we deleted last so update tail
        free_list_last = freed_node->prev;
    }

    // mark as free
    freed_node->free = TRUE;
    AllocHeader* prevOfFreed = freed_node->prev;
    // update pointers around node
    if(prevOfFreed) // if not head
    {
        prevOfFreed->next = freed_node->next;
    }
    else
    {
        // need to update
        free_list_head = freed_node->next;
    }

    if(freed_node->next) // if not tail
    {
        freed_node->next->prev = prevOfFreed;
    }
}

#endif