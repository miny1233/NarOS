#define NULL (void*)0
#define false 0
#define true 1

#define EOS 0

typedef unsigned int size_t;
typedef int ssize_t;

typedef unsigned long long int u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef u16 uint16_t;
typedef u32 uint32_t;
typedef u64 uint64_t;

typedef int idx_t;
typedef int dev_t;

#define likely(x)  __builtin_expect((x), 1)
#define unlikely(x)  __builtin_expect((x), 0)