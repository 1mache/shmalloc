#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

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

typedef struct
{
    u64 size;

} AllocHeader;

#define META_SIZE sizeof(AllocHeader)

typedef struct 
{
    byte* start;
    byte* end;
    u64   capacity;
} AllocBuffer;

void init_alloc_buffer(AllocBuffer* buffer, byte* resource, u64 capacity)
{
    buffer->start    = resource;
    buffer->end      = resource;
    buffer->capacity = capacity;
}

void* dumb_allocate(AllocBuffer* buffer ,u64 requested_bytes)
{
    u64 allocated_bytes = requested_bytes + META_SIZE;

    if((buffer->end - buffer->start) + allocated_bytes > buffer->capacity)
    {
        return NULL; // don't have space for allocation of this size 
    }
    
    u8* ret_address = buffer->end;
    // put the header at the head of the allocated block
    *((AllocHeader*)ret_address) = (AllocHeader){requested_bytes};
    ret_address += META_SIZE;
    buffer->end += allocated_bytes;
    // return the address of the start of memory right after header
    return ret_address;
}

void dumb_free(AllocBuffer* buffer ,void* ptr)
{
    if(!ptr) return;
    if(!buffer->start || !buffer->end) return; // invalid buffer
    if(buffer->start >= buffer->end) return; // empty buffer

    u8* ptr_to_alloc_block = (u8*)ptr - META_SIZE;
    //  total allocated     =    requested bytes                       + header size
    u64 allocated_bytes     = ((AllocHeader*)ptr_to_alloc_block)->size + META_SIZE;
    //  reset size
    ((AllocHeader*)ptr_to_alloc_block)->size = 0;  
    if(buffer->end - allocated_bytes < buffer->start)
    {
        return; // something went wrong, request free of bigger size than we have
    }
    buffer->end -= allocated_bytes;
}