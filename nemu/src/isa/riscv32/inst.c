/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "include/isa-def.h"
#include "isa.h"
#include "local-include/reg.h"
#include "local-include/csr.h"
#include "macro.h"
#include <stdbool.h>
#include <utils.h>
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J, TYPE_B, TYPE_R,
  TYPE_N, // none
};

static word_t ecall_func(word_t epc) {
    int excp_code;
    switch (cpu.priv) {
    case PRIV_MODE_M: excp_code = EXCP_M_CALL; break;
    case PRIV_MODE_U: excp_code = EXCP_U_CALL; break;
    default: panic("privi: %d not implement.", cpu.priv);
    }
    return isa_raise_intr(excp_code, epc);
} 

//TODO: need to support complete mcause
//      Now just set PRIV_MODE to mcause, 
//      and even the position of PRIV_MODE is wrong(should be )
static word_t mret_func(void) {
    mstatus_t mstatus_tmp = {.value = csr_read(CSR_MSTATUS)};
    // 将mstatus.MPIE还原到mstatus.MIE中
    mstatus_tmp.mie = mstatus_tmp.mpie;
    // 将mstatus.MPIE位置为1
    mstatus_tmp.mpie = 1;
    // 将处理器模式摄制成之前保存到MPP字段的处理器模式 mpp
    cpu.priv = mstatus_tmp.mpp;
    csr_write(CSR_MSTATUS, mstatus_tmp.value);
    
    return cpu.csr.mepc;
}

/** 
 * you may forget add sext for your instruction
 * if you don't add sext by macro to extend your imm.
 */
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
/**
 * instruction[31]      <--> imm[20]
 * instruction[19:12]   <--> imm[19:12]
 * instruction[20]      <--> imm[11]
 * instruction[30:21]   <--> imm[10:1]
 * left shift 1 bit     <--> imm[0]
 * ensure the lowest digit is cleared to zero
 * */
#define immJ() do { \
	*imm = SEXT(( \
            (BITS(i, 31, 31) << 19) | \
            (BITS(i, 19, 12) << 11) | \
            (BITS(i, 20, 20) << 10) | \
            (BITS(i, 30, 21)) \
            ) << 1, 21);  \
} while(0)
/**
 * instruction[31]      <--> imm[12]
 * instruction[ 7]      <--> imm[11]
 * instruction[30:25]   <--> imm[10:5]
 * instruction[11:8]    <--> imm[4:1]
 * left shift 1 bit     <--> imm[0]
 * ensure the lowest digit is cleared to zero
 */
#define immB() do { \
	*imm = SEXT(( \
            (BITS(i, 31, 31) << 11) | \
            (BITS(i, 7, 7)   << 10) | \
            (BITS(i, 30, 25) << 4) | \
            (BITS(i, 11, 8)) \
            ) << 1, 13);  \
} while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_R: src1R(); src2R();         break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

  /* 如何匹配相应指令，并执行相应指令 */
#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  /* 指令模式匹配和执行 */
  INSTPAT_START();
  /*                rs2   rs1        rd   opcode   */
  /* RV32I Base Integer Instructions */
  /* R type */
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2;);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = (src1 ^ src2) );
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = (src1 | src2) );
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << src2);
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> src2);
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (sword_t)src1 >> (sword_t)src2);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((sword_t)src1 < (sword_t)src2) );
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (src1 < src2) );

  /* I type */
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 5, 0));
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> BITS(imm, 5, 0));
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (sword_t)src1 >> BITS(imm, 5, 0));
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = ((sword_t)src1 < (sword_t)imm) );
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (src1 < (word_t)imm) );

  /* load */
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));

  /* store */
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  
  /* branch */
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, if (src1 == src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, if ((sword_t)src1 < (sword_t)src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, if ((sword_t)src1 >= (sword_t)src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, if (src1 < src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, if (src1 >= src2) s->dnpc = s->pc + imm);

  /* jump */
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, s->dnpc = s->pc + imm;
          IFDEF(CONFIG_FTRACE, {
            // jal label: jal ra offest
            // jump label, store the function return address in ra
            if (rd == 1) { 
              ftrace_func_call(s->pc, s->dnpc, false);
            }})
          R(rd) = s->pc + 4);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, volatile word_t t = s->pc + 4;
          s->dnpc = (src1 + imm) & ~(word_t)1;
          IFDEF(CONFIG_FTRACE, {
            // note the order of if and else if.
            if (s->isa.inst.val == 0x00008067) {
              ftrace_func_ret(s->pc);     // ret: jalr x0, 0(ra)
            } else if (rd == 1) {
              ftrace_func_call(s->pc, s->dnpc, false);   // normal call
            } else if (rd == 0 && imm == 0) {
              ftrace_func_call(s->pc, s->dnpc, true);   // jr rs
            }});
          R(rd) = t);

  /* jump and pc */
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);

  /* system */
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  
  /* CSR */
  // TODO: the following instructions need to be executed in M-mode?
  // need to consider the CSR level and cpu.priv
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I,
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            volatile word_t t = csr_read(imm); csr_write(imm, src1); R(rd) = t;
          });
  INSTPAT("??????? ????? ????? 101 ????? 11100 11", csrrwi , I,
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            R(rd) = csr_read(imm); 
            csr_write(imm, ZEXT(imm, 32));
          });
  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I, 
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            volatile word_t t = csr_read(imm); csr_write(imm, t | src1); R(rd) = t;
          });
  INSTPAT("??????? ????? ????? 110 ????? 11100 11", csrrsi , I, 
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            volatile word_t t = csr_read(imm); csr_write(imm, t | ZEXT(imm, 32)); R(rd) = t;
          });
  INSTPAT("??????? ????? ????? 011 ????? 11100 11", csrrc  , I, 
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            volatile word_t t = csr_read(imm); csr_write(imm, t & ~src1); R(rd) = t;
          });
  INSTPAT("??????? ????? ????? 111 ????? 11100 11", csrrci , I, 
          if (cpu.priv != PRIV_MODE_M) {
            isa_raise_intr(EXCP_ILLEGAL_INST, s->pc);
          } else {
            volatile word_t t = csr_read(imm); csr_write(imm, t & ~(ZEXT(imm, 32))); R(rd) = t;
          });

  
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall     , I,
          s->dnpc = ecall_func(s->pc););
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret      , R,
          s->dnpc = mret_func(););
  INSTPAT("0001000 00010 00000 000 00000 11100 11", sret      , R);// TODO
  INSTPAT("0001000 00101 00000 000 00000 11100 11", wfi       , R);
  INSTPAT("0000000 00000 00000 001 00000 00011 11", fence.i   , I);
  INSTPAT("0001001 ????? ????? 000 00000 11100 11", sfence.vma, R);

  /* M" Extension for Integer Multiplication and Division, Version 2.0 */
  /* RV32M Multiply Extension */
  // Complement multiplication
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R,
          R(rd) = (int64_t)(sword_t)src1 * (int64_t)(sword_t)src2);
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R,
          R(rd) = BITS((int64_t)(sword_t)src1 * (int64_t)(sword_t)src2, 63,32));
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R,
          R(rd) = BITS(((int64_t)(sword_t)src1 * (uint64_t)src2), 63,32));
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, 
          R(rd) = BITS(((uint64_t)src1 * (uint64_t)src2), 63,32));
  // mulw: only in RV64M

  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, 
          if (src2 == 0) R(rd) = -1;
          else {
            R(rd) = ((int32_t)src1 == INT32_MIN && (int32_t)src2 == -1) ?
                    src1 : ((int32_t)src1 / (int32_t)src2);
          });
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, 
          R(rd) = (src2 == 0) ? 0xffffffff : src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R,
          if (src2 == 0) R(rd) = src1;
          else {
            R(rd) = ((int32_t)src1 == INT32_MIN && (int32_t)src2 == -1) ?
                    0 : ((word_t)((sword_t)src1 % (sword_t)src2));
          });
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R,
          R(rd) = (src2 == 0) ? src1 : src1 % src2);

  /* RV32A Extension for Atomic Instructions*/
  // 31         25 24 20 19 15  14 12 11 7  6     0
  // 00010   aq rl 00000 rs1       010    rd      0101111
  INSTPAT("00010?? 00000 ????? 010 ????? 01011 11", lr.w   , R, 
          cpu.reserved_addr = src1;
          printf("[lr.w] reserved_addr: %x\n", cpu.reserved_addr);
          R(rd) = SEXT(Mr(src1 + imm, 4), 32);
          cpu.lr_valid = true;); 
  INSTPAT("00011?? ????? ????? 010 ????? 01011 11", sc.w   , R,
          int success = (cpu.reserved_addr == src1) && cpu.lr_valid;
          printf("[sc.w] reserved_addr: %x  src1: %x  vaild: %d  success: %d\n",
                 cpu.reserved_addr, src1, cpu.lr_valid, success);
          cpu.lr_valid = false;
          if (success) {
            Mw(src1, 4, src2); R(rd) = 0;
          } else {
            paddr_t paddr = R(rd);
            if (isa_mmu_check(paddr, 4, MEM_TYPE_WRITE) == MMU_TRANSLATE) {
              paddr = isa_mmu_translate(paddr, 4, MEM_TYPE_WRITE);
            }
            R(rd) = 1;
            //isa_raise_intr(EXCP_STORE_ACCESS, s->pc);
          }
         );

//   INSTPAT("00001?? ????? ????? 010 ????? 01011 11", amoswap.w, R,
  INSTPAT("00000?? ????? ????? 010 ????? 01011 11", amoadd.w , R,
          word_t t = Mr(src1, 4); Mw(src1, 4, t + src2); R(rd) = SEXT(t, 32););
  INSTPAT("00100?? ????? ????? 010 ????? 01011 11", amoxor.w , R,
          word_t t = Mr(src1, 4); Mw(src1, 4, t ^ src2); R(rd) = SEXT(t, 32););
  INSTPAT("01100?? ????? ????? 010 ????? 01011 11", amoand.w , R,
          word_t t = Mr(src1, 4); Mw(src1, 4, t & src2); R(rd) = SEXT(t, 32););
  INSTPAT("01000?? ????? ????? 010 ????? 01011 11", amoor.w  , R,
          word_t t = Mr(src1, 4); Mw(src1, 4, t | src2); R(rd) = SEXT(t, 32););

  INSTPAT("10000?? ????? ????? 010 ????? 01011 11", amomin.w , R,
          word_t t = Mr(src1, 4);
          Mw(src1, 4, (int32_t)t < (int32_t)src2 ? t : src2);
          R(rd) = SEXT(t, 32););
  INSTPAT("10100?? ????? ????? 010 ????? 01011 11", amomax.w , R,
          word_t t = Mr(src1, 4);
          Mw(src1, 4, (int32_t)t > (int32_t)src2 ? t : src2);
          R(rd) = SEXT(t, 32););
  INSTPAT("11000?? ????? ????? 010 ????? 01011 11", amominu.w, R,
          word_t t = Mr(src1, 4);
          Mw(src1, 4, (uint32_t)t < (uint32_t)src2 ? t : src2);
          R(rd) = SEXT(t, 32););
  INSTPAT("11100?? ????? ????? 010 ????? 01011 11", amomaxu.w, R,
          word_t t = Mr(src1, 4);
          Mw(src1, 4, (uint32_t)t > (uint32_t)src2 ? t : src2);
          R(rd) = SEXT(t, 32););

  /* add more... */
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  trace_inst(s->pc, s->isa.inst.val);
  return decode_exec(s);
}
