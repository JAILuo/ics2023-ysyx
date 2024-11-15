#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

static struct MiniRV32IMAState * core;
static uint32_t ram_amt = 64*1024*1024;
static uint8_t * ram_image = 0;

//static const char * dtb_file_name = 0;
static int dtb_ptr = 0;

// This is the functionality we want to override in the emulator.
//  think of this as the way the emulator's processor is connected to the outside world.
#define MINIRV32WARN( x... ) printf( x );
#define MINIRV32_DECORATE  static
#define MINI_RV32_RAM_SIZE ram_amt
#define MINIRV32_IMPLEMENTATION
//#define MINIRV32_POSTEXEC( pc, ir, retval ) { if( retval > 0 ) { if( fail_on_all_faults ) { printf( "FAULT\n" ); return 3; }
//#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( HandleControlStore( addy, val ) ) return val;
//#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = HandleControlLoad( addy );
//#define MINIRV32_OTHERCSR_WRITE( csrno, value ) HandleOtherCSRWrite( image, csrno, value );
//#define MINIRV32_OTHERCSR_READ( csrno, value ) value = HandleOtherCSRRead( image, csrno );


#include "mini-rv32ima.h"

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };

// 在DUT host memory的`buf`和REF guest memory的`addr`之间拷贝`n`字节,
// `direction`指定拷贝的方向, `DIFFTEST_TO_DUT`表示往DUT拷贝, `DIFFTEST_TO_REF`表示往REF拷贝
void difftest_memcpy(uint32_t addr, void *buf, size_t n, bool direction);
// `direction`为`DIFFTEST_TO_DUT`时, 获取REF的寄存器状态到`dut`;
// `direction`为`DIFFTEST_TO_REF`时, 设置REF的寄存器状态为`dut`;
void difftest_regcpy(void *dut, bool direction);
// 让REF执行`n`条指令
void difftest_exec(uint64_t n);
// 初始化REF的DiffTest功能
void difftest_init();


void difftest_init() {
    // 初始化模拟器状态
    core = (struct MiniRV32IMAState *)(ram_image + ram_amt - sizeof(struct MiniRV32IMAState));
    core->pc = 0x80000000;
    core->regs[10] = 0x00; // hart ID
    core->regs[11] = dtb_ptr + 0x80000000;
    core->extraflags |= 3; // Machine-mode.
    
    core->mstatus = 0x00001800;
}

void difftest_exec(uint64_t n) {
    uint64_t instrs_to_execute = n;
    while (instrs_to_execute > 0) {
        int instrs_per_flip = 1; // 每次只执行一条指令
        int ret = MiniRV32IMAStep(core, ram_image, 0, 0, instrs_per_flip); // 执行指令
        switch (ret) {
            case 0: // 正常执行
                break;
            case 1: // 需要睡眠，但在这里我们忽略
                break;
            case 3: // 终止执行
                instrs_to_execute = 0;
                break;
            case 0x7777: // 重启
                goto restart;
            case 0x5555: // 关闭
                printf("POWEROFF@0x%08x%08x\n", core->cycleh, core->cyclel);
                return;
            default: // 未知错误
                printf("Unknown failure\n");
                break;
        }
        instrs_to_execute -= instrs_per_flip;
    }
    return;

restart:
    // 重启逻辑，如果需要
    printf("Restarting...\n");
    // 添加重启模拟器的代码
}

// void difftest_exec(uint64_t n) {
//     uint64_t instrs_to_execute = n;
//     uint64_t lastTime = (fixed_update)?0:(GetTimeMicroseconds()/time_divisor);
//     while (instrs_to_execute > 0) {
//         int instrs_per_flip = (single_step) ? 1 : 1024;
//         int ret = MiniRV32IMAStep(core, ram_image, 0, elapsedUs, instrs_per_flip);
//         switch (ret) {
//             case 0: break;
//             case 1: if (do_sleep) MiniSleep(); break;
//             case 3: instrs_to_execute = 0; break;
//             case 0x7777: goto restart;    // syscon code for restart
//             case 0x5555: printf("POWEROFF@0x%08x%08x\n", core->cycleh, core->cyclel); return; // syscon code for power-off
//             default: printf("Unknown failure\n"); break;
//         }
//         instrs_to_execute -= instrs_per_flip;
//     }
// }

void difftest_memcpy(uint32_t addr, void *buf, size_t n, bool direction) {
    if (direction == DIFFTEST_TO_REF) {
        // 从DUT主机内存拷贝到REF客户内存
        for (size_t i = 0; i < n; i++) {
            ((uint8_t*)ram_image)[addr + i] = ((uint8_t*)buf)[i];
        }
    } else if (direction == DIFFTEST_TO_DUT) {
        // 从REF客户内存拷贝到DUT主机内存
        for (size_t i = 0; i < n; i++) {
            ((uint8_t*)buf)[i] = ((uint8_t*)ram_image)[addr + i];
        }
    }
}

void difftest_regcpy(void *dut, bool direction) {
    if (direction == DIFFTEST_TO_REF) {
        // 从DUT获取寄存器状态到REF
        struct MiniRV32IMAState *ref_state = (struct MiniRV32IMAState *)core;
        struct MiniRV32IMAState *dut_state = (struct MiniRV32IMAState *)dut;

        for (int i = 0; i < 32; i++) {
            ref_state->regs[i] = dut_state->regs[i];
        }
        ref_state->pc = dut_state->pc;
        // 同步CSR寄存器
        ref_state->mstatus = dut_state->mstatus;
        ref_state->mtvec = dut_state->mtvec;
        ref_state->mepc = dut_state->mepc;
        ref_state->mcause = dut_state->mcause;
    } else if (direction == DIFFTEST_TO_DUT) {
        // 从REF设置寄存器状态到DUT
        struct MiniRV32IMAState *ref_state = (struct MiniRV32IMAState *)core;
        struct MiniRV32IMAState *dut_state = (struct MiniRV32IMAState *)dut;

        for (int i = 0; i < 32; i++) {
            dut_state->regs[i] = ref_state->regs[i];
        }
        dut_state->pc = ref_state->pc;
        // 同步CSR寄存器
        dut_state->mstatus = ref_state->mstatus;
        dut_state->mtvec = ref_state->mtvec;
        dut_state->mepc = ref_state->mepc;
        dut_state->mcause = ref_state->mcause;
    }
}

void difftest_raise_intr(uint64_t NO) {
    if (NO >= 32) return;

    // 设置mip寄存器，标记中断为待处理
    core->mip |= (1ULL << NO);

    // 如果mie寄存器对应的中断使能位也设置，则触发中断
    if (core->mie & (1ULL << NO)) {
        // 模拟中断处理
        // 更新mcause寄存器，记录中断号
        core->mcause = NO;

        // 更新mepc寄存器，保存中断发生时的程序计数器
        core->mepc = core->pc;

        // 设置程序计数器到中断处理程序的地址
        // 这里假设mtvec寄存器已经设置了正确的中断向量基地址
        core->pc = core->mtvec + (NO * 4);

        // 更新mstatus寄存器，反映进入中断处理时的状态
        // 这里假设MIE位在mstatus寄存器中的位置是第3位
        core->mstatus &= ~(1ULL << 3); // 禁用中断
    }
}

