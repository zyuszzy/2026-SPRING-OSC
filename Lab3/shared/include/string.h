#ifndef STRING_H
#define STRING_H

# include "type.h"

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
int strlen(const char *s);
static inline uint32_t bswap32(uint32_t x){
    return ((x & 0x000000ff) << 24) |
           ((x & 0x0000ff00) << 8)  |
           ((x & 0x00ff0000) >> 8)  |
           ((x & 0xff000000) >> 24);
}
static inline uint64_t bswap64(uint64_t x){
    return ((x & 0x00000000000000ffULL) << 56) |
           ((x & 0x000000000000ff00ULL) << 40) |
           ((x & 0x0000000000ff0000ULL) << 24) |
           ((x & 0x00000000ff000000ULL) << 8)  |
           ((x & 0x000000ff00000000ULL) >> 8)  |
           ((x & 0x0000ff0000000000ULL) >> 24) |
           ((x & 0x00ff000000000000ULL) >> 40) |
           ((x & 0xff00000000000000ULL) >> 56);
}
static inline const void* align_up(const void* ptr, size_t align){
    return (const void*)(((uintptr_t)ptr + align - 1) & ~(align - 1));      // 公式：(addr + 3) & ~3  (假設 align 為 4)
}
int hextoi(const char* s, int n);
int align(int n, int byte);

#endif