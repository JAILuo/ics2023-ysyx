#include "common.h"
#include "device/alarm.h"
#include "device/map.h"
#include "isa-def.h"
#include <isa.h>
#include <stdbool.h>
#include <stdint.h>
#include "../local-include/csr.h"

#define CLINT_MSIP      (0x0000 / sizeof(clint_base[0]))
#define CLINT_MTIMECMP  (0x4000 / sizeof(clint_base[0]))
#define CLINT_MTIME     (0xBFF8 / sizeof(clint_base[0]))

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
       // but this is actually software (OS) task
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

    // notice, I don't implement mie in nanos-lite and am,
    // so nemu(hardware) do this this so that nanos-lite could switch different tasks.
    // mie_t mie = {
    //     .mtie = (clint_base[CLINT_MTIME] >= clint_base[CLINT_MTIMECMP])
    // };
    // csr_write(CSR_MIE, mie.value);
}

void update_clint() {
    //clint_base[CLINT_MTIME] += TIMEBASE / 10000;

    // why this can't run? endless loop in futex hash table 
    uint64_t uptime = get_time();
    clint_base[CLINT_MTIME] = uptime / US_PERCYCLE;
    //Log("uptime: %ld  CLINT_MTIME: %d  CLINT_MTIMECMP: %d",
    //    uptime, clint_base[CLINT_MTIME], clint_base[CLINT_MTIMECMP]);

    trigger_timer_interrupt();
    //cpu.INTR = true;
}

// static uint32_t mtimeh = 0;
// void update_clint() {
//     uint64_t uptime = get_time();
//     uint64_t new_mtime = uptime / US_PERCYCLE;
// 
//     // // 检查mtime 32-bits是否溢出
//     if (new_mtime < clint_base[CLINT_MTIME]) mtimeh++;
//     clint_base[CLINT_MTIME] = (uint32_t)new_mtime;
//     mtimeh = (uint32_t)(new_mtime >> 32);  // 更新mtimeh
//     clint_base[CLINT_MTIME + 1] = mtimeh;
// 
// 
//     //clint_base[CLINT_MTIME] += TIMEBASE / 10000;
//     // printf("mtime: 0x%x\n", clint_base[CLINT_MTIME]);
//     bool timer_interrupt = ((uint64_t)clint_base[CLINT_MTIME] >= 
//                             ((uint64_t)clint_base[CLINT_MTIMECMP] | 
//                              ((uint64_t)clint_base[CLINT_MTIMECMP + 1] << 32)));
//     // printf("mtime:    0x%016lx\n",
//     //       (uint64_t)clint_base[CLINT_MTIME] | ((uint64_t)clint_base[CLINT_MTIME + 1] << 32));
//     // printf("mtimecmp: 0x%016lx\n",
//     //        (uint64_t)clint_base[CLINT_MTIMECMP] | ((uint64_t)clint_base[CLINT_MTIMECMP + 1] << 32));
// 
// 
//     mip_t mip = {.value = csr_read(CSR_MIP)};
//     if (timer_interrupt) {
//         mip.mtip = 1;
//         csr_write(CSR_MIP, mip.value);
//     }
// 
//     //trigger_timer_interrupt();
// }
#endif

static void clint_io_handler(uint32_t offset, int len, bool is_write) {
    update_clint();
    // TODO: add offset access some basic register?
}

void init_clint(void) {
    clint_base = (uint32_t*)new_space(0x10000);
    add_mmio_map("clint", CONFIG_CLINT_MMIO, (uint8_t *)clint_base, 0x10000, clint_io_handler);
    add_alarm_handle(update_clint);
}

