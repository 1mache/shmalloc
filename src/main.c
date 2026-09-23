#include <stdio.h>
#include "shmalloc.h"
#include "prng.h"
#include "tests.c"

#define ARENA_SIZE Kb(1)

#define ENTER_KEY 10
#define ALLOC_KEY 65
#define FREE_KEY  66

int main()
{
    test_continous_allocations();
    test_continous_different_allocations();
    test_noncontinous_free();
    test_noncontinous_allocations();
    test_middle_merge_reuse();
    test_block_merge(); 
    test_block_split();
    
    printf("Meta size is %ld\n", META_SIZE);
    MemArena arena;
    
    memarena_init(&arena, Kb(2));
    printf("Arena initialized with arena size %ld\n", arena.capacity);
    
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
            ptr = (byte*)arena_shmalloc(&arena, 64);
            if(ptr == NULL) printf("No more arena space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptr + 64 - arena.start), arena.capacity);
        }
        if(ch == FREE_KEY)
        {
            if(!ptr) printf("Recent ptr not recorded or already freed");
            else
            {
                arena_free(&arena, ptr);
                printf("Free! Great success! Left %ld / %ld bytes\n", (arena.end - arena.start), arena.capacity);
                ptr = NULL;
            }
        }
    }

    memarena_free_all(&arena);
    memarena_destroy(&arena);
    
    return 0;
}

#define SHMALLOC_IMPLEMENTATION
#include "shmalloc.h"
#define PRNG_IMPLEMENTATION
#include "prng.h"