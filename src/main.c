#include <stdio.h>
#include "shmalloc.h"

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

int main()
{
    byte internal_buffer[Kb(1)];
    AllocBuffer buffer;

    init_alloc_buffer(&buffer, internal_buffer ,Kb(1));
    
    while(TRUE)
    {
        u8 dump = getchar();
        u8 ch = dump;
        while(dump != ENTER)
        {
            ch = dump;
            dump = getchar();
        }
        
        if(ch == ALLOC_KEY)
        {
            printf("Allocating 64 bytes...");
            byte* ptr = (byte*)dumb_allocate(&buffer, 64);
            if(ptr == NULL) printf("No more buffer space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptr + 64 - buffer.start), buffer.capacity);
        }

    }
    
    return 0;
}