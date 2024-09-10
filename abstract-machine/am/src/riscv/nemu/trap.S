#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#define PRIV_MODE_U 0
#define PRIV_MODE_S 1
#define PRIV_MODE_M 3

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#define CONTEXT_SIZE  ((NR_REGS + 3 + 2) * XLEN)
#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
#define OFFSET_NP     ((NR_REGS + 3) * XLEN)

.align 3
.globl __am_asm_trap
__am_asm_trap:
# switch to kernel stack
  csrrw sp, mscratch, sp   // (1) atomically exchange sp and mscratch
  bnez sp, create_context_space  // (2) take the branch if we trapped from user
  csrrw sp, mscratch, sp   // (3) if we trapped from kernel, restore the original sp
						   // after above 3 inst, sp must be ksp
# notice the conditional statements in assembly.
create_context_space:
  addi sp, sp, -CONTEXT_SIZE
  MAP(REGS, PUSH)
  # 先入栈保存用到的GPR, 也就是进入trap 前的相关寄存器
  # 如果后面save_kernel_context再保存的话，存的就是切换内核栈用到的一些寄存器了。

  # c->np = (ksp == 0) ? KERNEL : USER;
  csrr t0, mscratch
  beqz t0, save_kernel_stack

  csrr t0, mscratch    # exception from user
  li t1, PRIV_MODE_U   # c->np = USER
  j save_kernel_context

save_kernel_stack:
  mv t0, sp			   # exception from kernel
  li t1, PRIV_MODE_M   # c->np = KERNEL
  addi t0, t0, CONTEXT_SIZE

# 我之后要使用这个 sp 和 priv，把它入栈
# 但是又不能影响到要保存的上下文
# 能不能直接用这里的保存的上下文结构里的sp 和 t1？
# 有区别吗？有吧。
# 我切换到内核线程，那这个时候的栈指针应该指向的是内核栈保存的上下文。
# 他里面的各个寄存器应该不一样？
# 要不直接再扩大一下栈空间？
# 不对，一样的，保存之前都进行过同样的操作，应该可以.
# 哦，所以在刚开始保存的时候，就要开栈了。
# 哦，都帮我们写好了这个OFFSET_SP

save_kernel_context:
  # ksp = 0; support re-entry of CTE
  csrw mscratch, zero

  STORE t0, OFFSET_SP(sp) 
  STORE t1, OFFSET_NP(sp)

 csrr t0, mcause
 csrr t1, mstatus
 csrr t2, mepc

 STORE t0, OFFSET_CAUSE(sp)
 STORE t1, OFFSET_STATUS(sp)
 STORE t2, OFFSET_EPC(sp)

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp
  jal __am_irq_handle
  mv sp, a0

  # c->np != USER
  LOAD t0, OFFSET_NP(sp)
  li t1, PRIV_MODE_U
  bne t0, t1, restore_ksp_ok

  # c->np == USER
  # the value of ksp is the bottom of the kernel stack
  addi t0, sp, CONTEXT_SIZE
  csrw mscratch, t0

restore_ksp_ok:
  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  MAP(REGS, POP)

  # do not forget to restore the sp before entering the CTE
  LOAD sp, OFFSET_SP(sp)
  mret
