#include <float.h>
#include <klib.h>
#include <klib-macros.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    if (!s) return 0;

    size_t len = 0;
    while (*s != '\0') {
        len++;
    }
    return len;
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

void *memcpy(void *out, const void *in, size_t n) {
    // 确保out和in指向的内存地址不重叠
    if ((char *)out < (char *)in || (char *)out >= (char *)in + n) {
        // 没有重叠，直接复制
        for (size_t i = 0; i < n; ++i) {
            ((char *)out)[i] = ((char *)in)[i];
        }
    } else {
        // 有重叠，从后向前复制以避免数据被覆盖
        for (size_t i = n; i != 0; --i) {
            ((char *)out)[i - 1] = ((char *)in)[i - 1];
        }
    }
    return out;
}
void *memmove(void *dst, const void *src, size_t n) {
    // 检查dst和src是否重叠
    if ((src < dst && (char *)src + n <= (char *)dst) ||
        (dst < src && (char *)dst + n <= (char *)src)) {
        // 没有重叠，或者从dst到src的移动，直接复制
        return memcpy(dst, src, n);
    } else {
        // 有重叠，需要从后向前复制
        const char *s = (const char *)src + n - 1;
        char *d = (char *)dst + n - 1;
        for (size_t i = 0; i < n; ++i) {
            *d-- = *s--;
        }
    }
    return dst;
    //panic("no implemented");
}

/*
void *memmove(void *dest, const void *src, size_t n) {
    // 分配临时数组
    unsigned char temp[256]; 
    // 复制数据到临时数组
    memcpy(temp, src, n);

    // 复制数据从临时数组到目标地址
    memcpy(dest, temp, n);

    return dest;
}
*/

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
