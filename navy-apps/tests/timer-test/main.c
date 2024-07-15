#include <stdio.h>
#include <NDL.h>

int main() {
    NDL_Init(0);

    uint32_t time = NDL_GetTicks();
    size_t ms = 500;
    while (1) {
        while (time < ms) {
            time = NDL_GetTicks();
        }
        printf("second: %lu\n", ms / 1000);
        printf("ms: %lu\n", ms);
        ms += 500;
    }
}
