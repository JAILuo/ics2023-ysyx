#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "Vtop.h"
#include <verilated.h>
#include <verilated_vcd_c.h>
#include <nvboard.h>

void nvboard_bind_all_pins(Vtop* top);

int main(int argc,char **argv) {
    VerilatedContext* contextp = new VerilatedContext;
    contextp->commandArgs(argc, argv);

    //Verilated::commandArgs(argc,argv);

    Vtop *top = new Vtop("top");
    
    VerilatedVcdC *tfp = new VerilatedVcdC;
    Verilated::traceEverOn(true); // 打开trace
    top->trace(tfp, 99);
    tfp->open("build/wave.vcd");
    
    nvboard_bind_all_pins(top);
    nvboard_init();

    while(!Verilated::gotFinish()) {
        int a = rand() & 1;
        int b = rand() & 1;

        top->sw = 0;
        top->sw = (a << 0) | (b << 1); // a设置到第0位，b设置到第1位

        top->eval();

        tfp->dump(contextp->time()); //dump wave
		contextp->timeInc(1); //推动仿真时间

        printf("a = %d, b = %d, f = %d\n", a, b, (top->ledr && 0xffff));
        //assert(top->ledr == a ^ b);
        nvboard_update();
    }
    delete top;
    tfp->close();
    delete contextp;
    nvboard_quit();
    return 0;
}

