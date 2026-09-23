# shmalloc

A small `malloc`-style allocator written from scratch in C, as a learning project.

shmalloc doesn't replace the system `malloc`. It sits next to it. It never touches `brk`/`sbrk`, so it can't fight glibc over the program break. You give it a memory region, called an **arena**, and it allocates inside that region. The region can be pages mapped with `mmap` or a buffer you already own.

> **Platform:** Linux only (uses `mmap`/`munmap` and `sysconf`). Tested on Ubuntu.

## Features

- **Block list with first-fit search.** Every block has a header with its size, a free flag, and links to the previous and next blocks.
- **Block splitting.** If a free block is much bigger than the request, it gets split. The leftover part becomes a new free block.
- **Block merging (coalescing).** On `free`, the block merges with free neighbours on both sides. During allocation, runs of adjacent free blocks can merge to fit a bigger request.
- **Tail trimming.** Freeing the last block, plus any free blocks just before it, moves the arena's end pointer back so the space can be used again.
- **Arenas.** Reserve memory once at startup and do all your allocations inside it. You can also free everything at once with `memarena_free_all`.
- **Two ways to back an arena:**
  - *Owning:* the arena maps its own pages with `mmap` and unmaps them on destroy.
  - *Non-owning:* you pass in any buffer, for example `byte buf[4096]` on the stack.
- **Word alignment.** Every request is rounded up to `sizeof(void*)`.
- **Debug magic.** Each header stores a magic value. Freeing a pointer shmalloc didn't hand out, or freeing the same pointer twice, trips an `assert`.

## How it works

```
arena->start                                                  arena->end        start + capacity
     │                                                             │                    │
     ▼                                                             ▼                    ▼
     ┌────────┬──────────────┬────────┬──────────┬────────┬────────┐────────────────────┐
     │ header │  user data   │ header │  (free)  │ header │  data  │   untouched space  │
     └────────┴──────────────┴────────┴──────────┴────────┴────────┘────────────────────┘
               ▲
               └── pointer returned by arena_shmalloc
```

Each allocation is a `MetaHeader` followed by the user's bytes:

```c
typedef struct MetaHeader {
    u64 size;                 // payload size (excluding header)
    struct MetaHeader* prev;  // previous block in memory
    struct MetaHeader* next;  // next block in memory
    int _debug;               // magic value for sanity checks
    b8 free;
} MetaHeader;
```

`arena_shmalloc(arena, n)` does the following:
1. Rounds `n` up to the word size.
2. Walks the block list for the first free block that fits. If the fitting block is big enough, it is split.
3. If no single block fits, tries to merge a run of adjacent free blocks into one.
4. If that fails too, adds a new block at `arena->end`. If the arena is full, returns `NULL`.

`arena_free(arena, ptr)` does the following:
1. Steps back one header from `ptr` and checks the magic value.
2. Merges the block with a free previous neighbour and a free next neighbour.
3. If the block is now the last one, removes it from the tail and moves `arena->end` back.

## API

Include the header anywhere. In **exactly one** `.c` file, define `SHMALLOC_IMPLEMENTATION` before including it (the stb-style single-header pattern):

```c
#define SHMALLOC_IMPLEMENTATION
#include "shmalloc.h"
```

| Function | Description |
|---|---|
| `void memarena_init(MemArena* a, u64 capacity)` | `mmap` an arena. `capacity` is rounded up to a multiple of the 4 KiB page size. |
| `void memarena_init_nonown(MemArena* a, byte* buf, u64 capacity)` | Use a buffer you already own as the arena. |
| `void* arena_shmalloc(MemArena* a, u64 n)` | Allocate `n` bytes. Returns `NULL` when the arena is out of space. |
| `void arena_free(MemArena* a, void* ptr)` | Free a pointer from `arena_shmalloc`. `NULL` is a no-op. |
| `void memarena_free_all(MemArena* a)` | Reset the whole arena in one step. |
| `void memarena_destroy(MemArena* a)` | `munmap` an owning arena. Does nothing for non-owning arenas. |

## Usage

**With `mmap`-backed pages:**

```c
#include "shmalloc.h"

int main(void)
{
    MemArena arena;
    memarena_init(&arena, Kb(16));          // 4 pages via mmap

    int* nums = arena_shmalloc(&arena, 100 * sizeof(int));
    char* name = arena_shmalloc(&arena, 32);

    arena_free(&arena, nums);
    arena_free(&arena, name);

    memarena_destroy(&arena);               // munmap
    return 0;
}

#define SHMALLOC_IMPLEMENTATION
#include "shmalloc.h"
```

**With your own buffer (no syscalls):**

```c
byte buffer[4096];
MemArena arena;
memarena_init_nonown(&arena, buffer, sizeof(buffer));

void* p = arena_shmalloc(&arena, 64);
arena_free(&arena, p);
```

## Building & running

```bash
mkdir -p out
./build.sh        # gcc -std=c99 -Wall -Werror  -> out/main
./debug.sh        # same, plus -g -O0 and AddressSanitizer
./out/main
```

`out/main` runs the test suite first. The tests use a 1 KiB stack buffer and cover:
- sequential allocations
- random-sized allocations
- freeing in shuffled order
- allocating and freeing interleaved
- merging blocks in the middle of the arena
- merging many blocks into one big allocation
- splitting one big block

After the tests, `out/main` starts a small interactive demo on a 4 KiB `mmap` arena:

| Input | Action |
|---|---|
| `A` + Enter | allocate 64 bytes |
| `B` + Enter | free the most recent allocation |

## Project layout

```
src/
├── shmalloc.h   # the allocator (single-header library)
├── types.h      # fixed-width typedefs, Kb/Mb/Gb, MIN/MAX
├── prng.h       # PCG32 random generator (used by tests)
├── tests.c      # test suite
└── main.c       # runs tests, then the interactive demo
```

## Limitations

This is a learning project, not a production allocator.

- **One arena at a time.** The block list head and tail are file-level `static` globals, so two arenas in use at the same time will corrupt each other.
- **Not thread-safe.** There are no locks.
- **Assumes 4 KiB pages.** An `assert` fails on systems with a different page size.
- **Fixed size.** An arena never grows. When it fills up, `arena_shmalloc` returns `NULL`.
- **O(n) first-fit search.** Allocation walks every block.
- **No `realloc` or `calloc`.**

## Inspiration

- Arena allocators, and the idea of reserving memory up front and working inside that space.
- [PCG random number generator](https://www.pcg-random.org) (`prng.h`)
