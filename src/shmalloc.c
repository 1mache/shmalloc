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

#define ENTER       10
#define ALLOC_KEY   65

typedef struct
{
    u64 size;

} AllocHeader;

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

void* dumb_allocate(AllocBuffer* buffer ,u64 size_bytes)
{
    if((buffer->end - buffer->start) + size_bytes > buffer->capacity)
    {
        return NULL; // don't have space for allocation of this size 
    }
    
    u8* tmp = buffer->end;
    buffer->end += size_bytes;
    return tmp;
}