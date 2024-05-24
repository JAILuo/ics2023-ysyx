#include <float.h>
#include <klib.h>
#include <klib-macros.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  panic("Not implemented");
}

char *strcpy(char *dst, const char *src) {
    char *start = dst; 

    while ((*dst++ = *src++) != '\0');
    return start;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dst[i] = src[i];
    for ( ; i < n; i++)
        dst[i] = '\0';

   return dst;
}

/*
char *strncpy(char *dst, const char *src, size_t n) {
  panic("Not implemented");
}
*/

char *strcat(char *dst, const char *src) {
    char *end = dst;
    while (*end) {
        end++;
    }

    while (*src) {
        *end++ = *src++;
    }

    *end = '\0';

    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s1 == *s2 ) {
        s1++;
        s2++;
    } 
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    
    for (size_t i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }
    return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *str1 = (const unsigned char *)s1;
    const unsigned char *str2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            // if differ, return difference.
            return (int)str1[i] - (int)str2[i];
        }
    }
    return 0;
}
#endif
