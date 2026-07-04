#include "../include/string.h"

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }

    return *(const unsigned char *)a - *(const unsigned char *)b;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) {
        len++;
    }
    return len;
}

#include <stdint.h>

uint32_t atoi_hex(const char *s) {
    uint32_t result = 0;

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        char c = *s;
        uint32_t value;

        if (c >= '0' && c <= '9') {
            value = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            value = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            value = c - 'A' + 10;
        } else {
            break;
        }

        result = (result << 4) | value;
        s++;
    }

    return result;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;

    for (int i = 0; haystack[i]; i++) {
        int j = 0;
        while (needle[j] && haystack[i + j] == needle[j]) {
            j++;
        }

        if (!needle[j]) {
            return (char *)&haystack[i];
        }
    }

    return 0;
}
