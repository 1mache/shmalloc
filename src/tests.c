#include <stdio.h>
#include "shmalloc.c"

#define TEST_BUFFER_SIZE Kb(1)
#define TEST_ALLOCATION 64

b8 test_continous_allocations()
{
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
    printf("%d pointers fit\n", num_ptrs);

    for(int i = num_ptrs-1; i >= 0; --i)
    {
        dumb_free(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");

    return TRUE;
}