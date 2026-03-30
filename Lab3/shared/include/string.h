#ifndef STRING_H
#define STRING_H

# include "type.h"

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int n);
int strlen(const char *s);
uint32_t bswap32(uint32_t x);
uint64_t bswap64(uint64_t x);
const void* align_up(const void* ptr, size_t align);
int hextoi(const char* s, int n);
int align(int n, int byte);

#endif