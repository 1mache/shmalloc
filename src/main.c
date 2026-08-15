#include <stdio.h>
#include "shmalloc.h"

#define BUFFER_SIZE Kb(1)
#define ENTER       10
#define ALLOC_KEY   65

u8 buffer[Kb(1)];

void* dumb_allocate(u64 amount)
{
    static u8* bufferEnd = buffer;
    if((bufferEnd - buffer) + amount > BUFFER_SIZE)
    {
        return NULL;
    }
    
    u8* tmp = bufferEnd;
    bufferEnd += amount;
    return tmp;
}

int main()
{
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
            u8* ptr = (u8*)stack_allocate(64);
            if(ptr == NULL) printf("No more buffer space\n");
            else printf("Great success! Used %ld / %ld bytes\n", (ptr + 64 - buffer), BUFFER_SIZE);
        }

    }
    
    return 0;
}