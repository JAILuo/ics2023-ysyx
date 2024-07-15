#include <stdio.h>
#include <sys/time.h>

int main() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    size_t ms = 500;
    while (1) {
        while ((tv.tv_sec * 1000 + tv.tv_usec / 1000) < ms) {
            gettimeofday(&tv, NULL);
        }
        printf("tv.tv_sec: %ld ", tv.tv_sec);
        printf("ms = %lu\n", ms);
        ms += 500;
    }
}
