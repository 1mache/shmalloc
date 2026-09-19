#include <stdio.h>
#include "shmalloc.h"
#include "prng.h"
#include "tests.c"

#define BUFFER_SIZE 96

#define ENTER_KEY 10
#define ALLOC_KEY 65
#define FREE_KEY  66

int main()
{
    test_continous_allocations();
    test_continous_different_allocations();
    test_noncontinous_free();
    test_middle_merge_reuse();

    printf("Buffer initialized with buffer size %ld\n", (u64)BUFFER_SIZE);
    byte internal_buffer[BUFFER_SIZE];
    MemBuffer buffer;

    membuffer_init(&buffer, internal_buffer ,BUFFER_SIZE);
    
    byte* ptr = NULL;
    while(TRUE)
    {
        u8 dump   = getchar();
        u8 ch     = dump;
        
        while(dump != ENTER_KEY)
        {
            ch = dump;
            dump = getchar();
        }
        
        if(ch == ALLOC_KEY)
        {
            printf("Allocating 64 bytes...");
            ptr = (byte*)shmalloc_buffered(&buffer, 64);
            if(ptr == NULL) printf("No more buffer space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptr + 64 - buffer.start), buffer.capacity);
        }
        if(ch == FREE_KEY)
        {
            if(!ptr) printf("Recent ptr not recorded or already freed");
            else
            {
                free_buffered(&buffer, ptr);
                printf("Great success! Left %ld / %ld bytes\n", (buffer.end - buffer.start), buffer.capacity);
                ptr = NULL;
            }
        }

    }
    
    return 0;
}

#define SHMALLOC_IMPLEMENTATION
#include "shmalloc.h"
#define PRNG_IMPLEMENTATION
#include "prng.h"