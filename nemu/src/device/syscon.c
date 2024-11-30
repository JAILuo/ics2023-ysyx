#include <stdint.h>
#include <stdbool.h>
#include <utils.h>
#include <device/map.h>

static uint8_t *syscon_base = NULL;
#define SYSCON_SIZE (0x1000)

static void syscon_io_handler(uint32_t offset, int len, bool is_write) {
    // Fake syscon handler, empty now
    return;
}

void init_syscon(void) {
    syscon_base = new_space(SYSCON_SIZE);
    add_mmio_map("syscon", CONFIG_SYSCON_MMIO, syscon_base, SYSCON_SIZE, syscon_io_handler);
}

