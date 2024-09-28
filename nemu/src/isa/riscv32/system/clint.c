#include "common.h"
#include "device/alarm.h"
#include "device/map.h"
#include "isa-def.h"
#include <isa.h>
#include <stdbool.h>
#include <stdint.h>
#include "../local-include/csr.h"

#define CLINT_MSIP     (0x0000 / sizeof(clint_base[0])) 
#define CLINT_MTIMECMP (0x4000 / sizeof(clint_base[0]))
#define CLINT_MTIME    (0xBFF8 / sizeof(clint_base[0]))
#define TIMEBASE 1000000ul
#define US_PERCYCLE (1000000 / TIMEBASE)

static uint32_t *clint_base = NULL;

//#define test

#ifdef test
void update_clint() {
    mip_t mip = {.value = csr_read(CSR_MIP)};
    mie_t mie = {.value = csr_read(CSR_MIE)};
    switch (cpu.INTR) {
    case 1: 
        mip.mtip = 1;
        csr_write(CSR_MIP, mip.value);

       // For testing, the hardware also writes mie here,
       // but this is actually a software (OS) task
        mie.mtie = 1;
        csr_write(CSR_MIE, mie.value);

       //printf("mie: " FMT_WORD "\nmip: " FMT_WORD "\n", mie.value, mip.value);
       break;
    case 2: break;
    default: cpu.INTR = 0;break;
    }
    //mip->mtip = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP]);
}
#else

static void trigger_timer_interrupt() {
    mip_t mip = {
        .mtip = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP])
    };
    csr_write(CSR_MIP, mip.value);

    mie_t mie = {
        .mtie = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP])
    };
    csr_write(CSR_MIE, mie.value);
}

// static void trigger_software_interrupt() {
//     mip_t mip = {
//         .msip = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP])
//     };
//     csr_write(CSR_MIP, mip.value);
// 
//     mie_t mie = {
//         .msie = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP])
//     };
//     csr_write(CSR_MIE, mie.value);
// }

void update_clint() {
    uint64_t uptime = get_time();
    clint_base[CLINT_MTIME] = uptime / US_PERCYCLE;

    trigger_timer_interrupt();
    //trigger_software_interrupt();
    cpu.INTR = true;
}
#endif

static void clint_io_handler(uint32_t offset, int len, bool is_write) {
    update_clint();
}

void init_clint(void) {
    clint_base = (uint32_t*)new_space(0x10000);
    add_mmio_map("clint", CONFIG_CLINT_MMIO, (uint8_t *)clint_base, 0x10000, clint_io_handler);
    add_alarm_handle(update_clint);
}
