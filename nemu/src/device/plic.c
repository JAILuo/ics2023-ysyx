#include "macro.h"
#include <stdint.h>
#include <stdbool.h>
#include <utils.h>
#include <device/map.h>

static uint8_t *plic_base = NULL;
#define PLIC_SIZE (0x10) 
// random size...

static void plic_io_handler(uint32_t offset, int len, bool is_write) {
    // Fake plic handler, empty now
    return;
}

void init_plic() {
    plic_base = new_space(PLIC_SIZE); // TOO MUCH
    add_mmio_map("plic", CONFIG_PLIC_ADDRESS, plic_base, PLIC_SIZE, plic_io_handler);
}
