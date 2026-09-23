#include <stdio.h>
#include "shmalloc.h"
#include <assert.h>
#include "prng.h"
#include "tests.c"

#define ARENA_SIZE Kb(4)
#define ALLOC_SIZE 64

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
    
    memarena_init(&arena, ARENA_SIZE);
    assert(arena.capacity == (u64)ARENA_SIZE && "Pick an ARENA_SIZE that is multiple of page size");
    printf("Arena initialized with arena size %ld\n", arena.capacity);
    
    byte* ptrs[ARENA_SIZE/ALLOC_SIZE];
    int lastptr = -1;
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
            ptrs[++lastptr] = (byte*)arena_shmalloc(&arena, 64);
            if(ptrs[lastptr] == NULL) printf("No more arena space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptrs[lastptr] + ALLOC_SIZE - arena.start), arena.capacity);
        }
        if(ch == FREE_KEY)
        {
            if(lastptr < 0) printf("Empty arena!\n");
            else
            {
                arena_free(&arena, ptrs[lastptr--]);
                printf("Free! Great success! Left %ld / %ld bytes\n", (arena.end - arena.start), arena.capacity);
                ptrs[lastptr+1] = NULL;
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