#include <stdio.h>
#include "shmalloc.c"

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