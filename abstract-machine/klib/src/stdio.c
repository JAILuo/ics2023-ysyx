#include <klib.h>
#include <stdbool.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <limits.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*
static void reverse(char *start, int len) {
    char *end = start + len - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}
*/

int itoa(int n, char *out, int base) {
    assert(out && base >= 2 && base <= 36);
    if (n == INT_MIN) {
        // INT_MIN is a special case, needs special handling
        strcpy(out, "-2147483648");
        return 11;
    }

    int len = 0;
    bool is_neg = n < 0;
    if (is_neg) {
        n = -n;
    }
    char buffer[33]; // Enough for 32-bit int base 2 to base 36
    char *ptr = &buffer[sizeof(buffer) - 1];
    *ptr = '\0';

    do {
        int digit = n % base;
        *--ptr = (digit < 10) ? '0' + digit : 'a' + (digit - 10);
        len++;
    } while ((n /= base) != 0);

    if (is_neg) {
        *--ptr = '-';
        len++;
    }

    strcpy(out, ptr);
    return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
    assert(out);
    assert(fmt);
  char *start = out;
  char *str = NULL;

  for (; *fmt != '\0'; ++fmt) {
    if (*fmt != '%') {
      *out = *fmt;
      ++out;
    } else {
      switch (*(++fmt)) {
      case '%': *out = *fmt; ++out; break;
      case 'd': out += itoa(va_arg(ap, int), out, 10); break;
      case 's':
        str = va_arg(ap, char*);
        while (*str)
            *out++ = *str++;
        break;
      }
    }
  }

  *out = '\0';
  return out - start;
}

int printf(const char *fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);
    for (int i = 0; i < len; i++) {
        putch(buf[i]);
    }
    return 0; // 返回值通常为写入的字符数，这里简化处理
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
            case '%': *out = *fmt; out++; break;
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
