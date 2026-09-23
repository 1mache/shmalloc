# shmalloc

My attempt at writing `malloc` from scratch in C, to find out what actually happens when you ask for memory.

> Linux only. Tested on Ubuntu.

## What I built

I started small. I made a `byte buffer[4096]` on the stack and wrote an allocator that hands out pieces of it. Each allocation gets a small header in front of it that stores the block's size, whether it's free, and links to the blocks before and after it. Then I added features one by one:

- **Free list.** Freed blocks get reused instead of being thrown away.
- **Splitting.** If a free block is way bigger than the request, I cut it in two and keep the leftover part free.
- **Merging.** When free blocks sit next to each other, I combine them so a bigger allocation can fit there later.

After that I got interested in **arenas**: you grab all the memory you need at startup and do all your allocating inside that space. So I added a way to request pages up front with `mmap` and use them instead of an external buffer.

I used `mmap` and not `sbrk` on purpose. I didn't want to *replace* `malloc`, just build my own version that could live next to it. `sbrk` moves the program break, which glibc's `malloc` also uses, so the two would step on each other. `mmap` pages belong only to me, so there's no conflict.

```c
MemArena arena;
memarena_init(&arena, Kb(16));   // 4 pages from mmap

int*  nums = arena_shmalloc(&arena, 100 * sizeof(int));
char* name = arena_shmalloc(&arena, 32);

arena_free(&arena, nums);
arena_free(&arena, name);

memarena_destroy(&arena);        // munmap
```

The old buffer mode still works too:

```c
byte buffer[4096];
MemArena arena;
memarena_init_nonown(&arena, buffer, sizeof(buffer));

void* p = arena_shmalloc(&arena, 64);
arena_free(&arena, p);
```

## Run it

```bash
mkdir -p out && ./build.sh && ./out/main
```

This runs the tests, then starts a small interactive demo: press up arrow + Enter to allocate, down arrow + Enter to free.

## Not done / known limits

This is a learning project, so it's missing a lot: only one arena at a time (the list lives in globals), not thread-safe, no `realloc`, and it assumes 4 KiB pages.
