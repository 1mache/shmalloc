#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "shmalloc.c"
#include "prng.c"

#define TEST_BUFFER_SIZE Kb(1)
#define TEST_ALLOCATION 64

void test_continous_allocations()
{
    printf("TEST: test_continous_allocations\n");

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemDummyBuffer buffer;

    init_alloc_buffer(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;

    void* ptr = dumb_allocate(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;
        ptr = dumb_allocate(&buffer, TEST_ALLOCATION);
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    for(int i = num_ptrs-1; i >= 0; --i)
    {
        dumb_free(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");

    printf("All freed\n");
}

void test_continous_different_allocations()
{
    printf("TEST: test_continous_different_allocations\n");
    pcg32_seed_random(time(NULL), getpid());
    
    u32 min_alloc_size = TEST_ALLOCATION;
    u32 max_alloc_size = TEST_ALLOCATION * 4;
    u32 cur_alloc_size = min_alloc_size;

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemDummyBuffer buffer;

    init_alloc_buffer(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;

    void* ptr = dumb_allocate(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;

        u64 how_much = min_alloc_size + (pcg32_random() % (max_alloc_size - min_alloc_size));
        printf("Allocating %ld bytes\n", how_much); 
        ptr = dumb_allocate(&buffer, how_much); 
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    for(int i = num_ptrs-1; i >= 0; --i)
    {
        dumb_free(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");
    printf("All freed\n");
}

void test_noncontinous_free()
{
    printf("TEST: test_noncontinous_free\n");
    pcg32_seed_random(time(NULL), getpid());

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemDummyBuffer buffer;
    init_alloc_buffer(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;
    
    void* ptr = dumb_allocate(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;
        ptr = dumb_allocate(&buffer, TEST_ALLOCATION);
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    // shuffle the array
    for(int i = 0; i < num_ptrs; ++i)
    {
        int j = pcg32_boundedrand(num_ptrs);
        void* tmp = ptrs[i];
        ptrs[i] = ptrs[j];
        ptrs[j] = tmp;
    }

    // free in shuffled order
    for(int i = num_ptrs-1; i >= 0; --i)
    {
        dumb_free(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");
    printf("All freed\n");
}