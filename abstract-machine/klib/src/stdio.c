#include <float.h>
#include <klib.h>
#include <stdbool.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

static void reverse(char *start, int len) {
    if (start && len > 0) {
        char *end = start + len - 1;
        while (start < end) {
            char temp = *start;
            *start = *end;
            *end = temp;

            start++;
            end--;
        }
    }
}

int itoa(int n, char *out, int base) { 
    assert(out);
    int len = 0;
    bool is_neg = false;
    if (is_neg) {
        n = -n;
    }
    do {
        int digit = n % base;
        *out++ = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
        len++;
    } while ((n /= base) > 0);

    if (is_neg) {
        *out = '-';
        len++;
    }
    *out = '\0';
    reverse(out - len, len);

    return len;
}

int sprintf(char *out, const char *fmt, ...) {
    assert(out);
    assert(fmt);
    va_list args;
    va_start(args, fmt);
    
    char *start = (char *)fmt;
    char *str = NULL;
    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            *out++ = *fmt;
        } else {
            switch (*(++fmt)) {
                case '%': *out = *fmt;out++; break;
                case 'd':
                    out += itoa(va_arg(args, int), out, 10); break;
                case 's':
                    str = va_arg(args, char*);
                    while (*str)
                        *out++ = *str++;
                    break;
                default: *out++ = *fmt; break;
            }
        }
    }
    *out = '\0';
    va_end(args);
    return out - start;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
