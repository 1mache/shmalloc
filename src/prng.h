#ifndef PRNG_H
#define PRNG_H

// based on http://www.pcg-random.org C Implementation.
#include "types.h"

// initstate = start pos on track 
// initseq   = track
typedef struct pcg32_state {   
    u64 state; ;
    u64 inc;                    
} pcg32_state;

#define PCG32_INITIALIZER { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

static pcg32_state s_prng = PCG32_INITIALIZER;

void pcg32_seed_random(u64 initstate, u64 initseq);
void pcg32_seed_random_r(pcg32_state* rng, u64 initstate, u64 initseq);

u32 pcg32_random(void);
u32 pcg32_random_r(pcg32_state* rng);

u32 pcg32_boundedrand(u32 bound);
u32 pcg32_boundedrand_r(pcg32_state* rng, u32 bound);

#endif // PRNG_H

#ifdef PRNG_IMPLEMENTATION

void pcg32_seed_random(u64 initstate, u64 initseq)
{
    pcg32_seed_random_r(&s_prng, initstate, initseq);
}

void pcg32_seed_random_r(pcg32_state* rng, u64 initstate, u64 initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

u32 pcg32_random(void)
{
    return pcg32_random_r(&s_prng);    
}

u32 pcg32_random_r(pcg32_state* rng)
{
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    u32 rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 pcg32_boundedrand(u32 bound)
{
    return pcg32_boundedrand_r(&s_prng, bound);
}

u32 pcg32_boundedrand_r(pcg32_state* rng, u32 bound)
{
    u32 threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In 
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        u32 r = pcg32_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}

#endif // PRNG_IMPLEMENTATION