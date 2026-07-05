#ifndef LOS_STRING_H
#define LOS_STRING_H

#include <stddef.h>
#include <stdint.h>

int strcmp(const char *a, const char *b);
size_t strlen(const char *s);
uint32_t atoi_hex(const char *s);

int strncmp(const char *a, const char *b, int n);

#endif
char *strstr(const char *haystack, const char *needle);
