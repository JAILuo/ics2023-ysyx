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

#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <debug.h>

/* Sv32 */
/*
#define VA_VPN_1(addr) ((addr >> 22) && 0x000003ff)
#define VA_VPN_0(addr) ((addr >> 12) && 0x000003ff)
#define VA_OFFSET(addr) (addr && 0x00000fff)

#define PTE_V(entry) (entry && 0x1)
#define PTE_R(entry) ((entry >> 1) && 0x1)
#define PTE_W(entry) ((entry >> 2) && 0x1) 
#define PTE_X(entry) ((entry >> 3) && 0x1) 
#define PTE_U(entry) ((entry >> 4) && 0x1)
#define PTE_G(entry) ((entry >> 5) && 0x1) 
#define PTE_A(entry) ((entry >> 6) && 0x1) 
#define PTE_D(entry) ((entry >> 7) && 0x1)
#define PTE_PPN(entry) ((entry >> 12) && 0x000fffff)


typedef paddr_t PTE;

*/
// 对内存区间为[vaddr, vaddr + len), 类型为type的内存访问进行地址转换
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
    /*
    paddr_t pt_base_reg = cpu.csr.satp;


    PTE pt_dir_base = (pt_base_reg << 12) + (VA_VPN_1(vaddr) << 2);
    PTE pte_1_addr = paddr_read(pt_dir_base, 4);
    Log("pt_dir_base:%p pte_1_addr:%d", pt_dir_base, pte_1_addr);
    Assert(PTE_V(pte_1_addr) != 0, "pt_dir_base is unvalid.");
    
    PTE pte_ppn_base = (PTE_PPN(pte_1_addr) << 12);
    PTE pte_2_addr = pte_ppn_base + (VA_VPN_0(vaddr) << 2);
    Log("pt_ppn_base:%p pte_2_addr:%d", pt_ppn_base, pte_2_addr);
    Assert(PTE_V(pte_2_addr) != 0, "pt_dir_base is unvalid.");
    
    PTE paddr_base = PTE_PPN(pte_2_addr);
    paddr_t paddr = paddr_base | VA_OFFSET(vaddr);

*/
  return MEM_RET_FAIL;
}
