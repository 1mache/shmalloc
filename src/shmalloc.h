#ifndef SHMALLOC_H
#define SHMALLOC_H

#include <stdint.h>

typedef int8_t   i8 ;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8 ;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   b8;
#define TRUE  1
#define FALSE 0

#define Kb(n) ((u64)(n) << 10)
#define Mb(n) ((u64)(n) << 20)
#define Gb(n) ((u64)(n) << 30)

#define MAX(a,b) (((a) > (b)) ? (a):(b))
#define MIN(a,b) (((a) < (b)) ? (a):(b))

void say_sum(const char* str);

#endif