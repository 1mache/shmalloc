#include <stdio.h>
#include "shmalloc.c"

int main()
{
    byte internal_buffer[Kb(1)];
    AllocBuffer buffer;

    init_alloc_buffer(&buffer, internal_buffer ,Kb(1));
    
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
            ptr = (byte*)dumb_allocate(&buffer, 64);
            if(ptr == NULL) printf("No more buffer space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptr + 64 - buffer.start), buffer.capacity);
        }
        if(ch == FREE_KEY)
        {
            if(!ptr) printf("Recent ptr not recorded or already freed");
            else
            {
                dumb_free(&buffer, ptr);
                printf("Great success! Left %ld / %ld bytes\n", (buffer.end - buffer.start), buffer.capacity);
                ptr = NULL;
            }
        }

    }
    
    return 0;
}