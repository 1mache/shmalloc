#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "shmalloc.h"
#include "prng.h"

#define TEST_BUFFER_SIZE Kb(1)
#define TEST_ALLOCATION 49

void test_continous_allocations()
{
    printf("TEST: test_continous_allocations\n");

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemBuffer buffer;

    membuffer_init(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;

    void* ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;
        ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    for(int i = num_ptrs-1; i >= 0; --i)
    {
        free_buffered(&buffer, ptrs[i]);
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
    MemBuffer buffer;

    membuffer_init(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;

    void* ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;

        u64 how_much = min_alloc_size + (pcg32_random() % (max_alloc_size - min_alloc_size));
        printf("Requested shmalloc of %ld bytes\n", how_much); 
        byte* tmp = buffer.end;
        ptr = shmalloc_buffered(&buffer, how_much);
        if(ptr)
        {
            printf("Actually allocated %ld of data\n", (u64)(buffer.end-tmp)-META_SIZE);
        }
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    for(int i = num_ptrs-1; i >= 0; --i)
    {
        free_buffered(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");
    printf("All freed\n");
}

void test_noncontinous_free()
{
    printf("TEST: test_noncontinous_free\n");
    pcg32_seed_random(time(NULL), getpid());

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemBuffer buffer;
    membuffer_init(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;
    
    void* ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;
        ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
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
        free_buffered(&buffer, ptrs[i]);
    }

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");
    printf("All freed\n");
}

void test_middle_merge_reuse()
{
    printf("TEST: test_middle_merge_reuse\n");

    byte internal_buffer[TEST_BUFFER_SIZE];
    MemBuffer buffer;

    membuffer_init(&buffer, internal_buffer, TEST_BUFFER_SIZE);

    void* ptrs[TEST_BUFFER_SIZE/TEST_ALLOCATION];
    int num_ptrs = 0;

    void* ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    while(ptr)
    {
        ptrs[num_ptrs] = ptr;
        ++num_ptrs;
        ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION);
    }
    printf("%d ptrs fit into the buffer\n", num_ptrs);

    // pick two memory-adjacent blocks somewhere in the middle (not first, not last)
    int mid = num_ptrs / 2;
    printf("Freeing blocks %d and %d (should be memory-adjacent)\n", mid, mid + 1);

    if (mid == 0 || mid+1 >= num_ptrs-1)
    {
        printf("Insufficient num_ptr for test. Freeing\n");
        free_all(&buffer);
        return;
    }

    void* expected_addr = ptrs[mid];

    free_buffered(&buffer, ptrs[mid + 1]);
    free_buffered(&buffer, ptrs[mid]);

    // the two freed x-byte blocks (2x-byte + 2 headers of space) should merge into
    // one free block big enough for a single 2x-byte allocation
    void* merged_ptr = shmalloc_buffered(&buffer, TEST_ALLOCATION * 2);

    assert(merged_ptr == expected_addr && "128b allocation did not land in the merged middle gap");

    printf("128b allocation correctly reused merged middle blocks\n");

    // cleanup: free everything else so the buffer can return to empty
    for(int i = 0; i < num_ptrs; ++i)
    {
        if(i == mid || i == mid + 1) continue; // already freed
        free_buffered(&buffer, ptrs[i]);
    }
    free_buffered(&buffer, merged_ptr);

    assert(buffer.start == buffer.end && "Buffer did not return to empty state");
    printf("All freed\n");
}