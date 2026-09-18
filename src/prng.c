// based on http://www.pcg-random.org C Implementation.
#include "types.h"

typedef struct prng_state {   
    u64 state; ;
    u64 inc;                    
} prng_state;