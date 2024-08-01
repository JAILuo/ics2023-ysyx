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
#include <stdint.h>

/* Sv32 */
#define VA_VPN_1(addr) ((addr >> 22) & 0x000003ff)
#define VA_VPN_0(addr) ((addr >> 12) & 0x000003ff)
#define VA_OFFSET(addr) (addr & 0x00000fff)

#define PTE_V(entry) (entry & 0x1)
#define PTE_R(entry) ((entry >> 1) & 0x1)
#define PTE_W(entry) ((entry >> 2) & 0x1) 
#define PTE_X(entry) ((entry >> 3) & 0x1) 
#define PTE_U(entry) ((entry >> 4) & 0x1)
#define PTE_G(entry) ((entry >> 5) & 0x1) 
#define PTE_A(entry) ((entry >> 6) & 0x1) 
#define PTE_D(entry) ((entry >> 7) & 0x1)
#define PTE_PPN(entry) ((entry >> 12) & 0x000fffff)

#define SATP_PPN(entry) ((entry >> 10) & 0x003fffff)

typedef paddr_t PTE;

// 对内存区间为[vaddr, vaddr + len), 类型为type的内存访问进行地址转换
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
    paddr_t pt_base_reg = cpu.csr.satp;
   
    //printf("vaddr: 0x%x\n", vaddr);
    
    // (satp.PPN * PGSIZE) + (VA[31:22] * PTESIZE)
    //PTE pt_dir_base = (SATP_PPN(pt_base_reg) << 12);
    PTE pt_dir_base = (pt_base_reg << 12);
    PTE pt_dir_offset = (VA_VPN_1(vaddr) << 2);
    PTE pt_dir = pt_dir_base + pt_dir_offset;
    //if (vaddr < 0x80000000) {
    //    printf("vaddr: 0x%x\n", vaddr);
    //    printf("pt_base_reg: 0x%x\n", pt_base_reg);
    //    printf("[pt_dir_base]: 0x%x  [pt_dir_offset]: 0x%x  [pt_dir]: 0x%x\n",
    //        pt_dir_base, pt_dir_offset, pt_dir);
    //}
    PTE pte_1_addr = paddr_read(pt_dir, 4);
    //if (vaddr < 0x80000000)
    //    printf("pte_1_addr: 0x%x\n", pte_1_addr);
    
    Assert(PTE_V(pte_1_addr) != 0, "pt_dir is unvalid.");
    
    // (PTE.PPN * 4096) + (VA[21:12] * 4)
    PTE pt_2_base = (PTE_PPN(pte_1_addr) << 12);
    PTE pt_2_offset = (VA_VPN_0(vaddr) << 2);
    PTE pte_2_addr = pt_2_base + pt_2_offset;
    PTE pte_2 = paddr_read(pte_2_addr, 4);
    //if (vaddr < 0x80000000) {
    //    printf("pt_2_base: 0x%x  pt_2_offset: 0x%x  pte_2_addr: 0x%x  pte_2: 0x%x\n",
    //    pt_2_base, pt_2_offset, pte_2_addr, pte_2);
    //}
    Assert(PTE_V(pte_2) != 0, "pt_2 is unvalid.");
   
    switch (type) {
        case MEM_TYPE_IFETCH: if (PTE_X(pte_2) == 0) assert(0); break;
        case MEM_TYPE_READ:   if (PTE_R(pte_2) == 0) assert(0); break;
        case MEM_TYPE_WRITE:  if (PTE_W(pte_2) == 0) assert(0); break;
        default: assert(0); break;
  }

    // (LeafPTE.PPN * 4096) + (VA[11:0])
    PTE paddr_base = (PTE_PPN(pte_2) << 12);
    paddr_t paddr = paddr_base | VA_OFFSET(vaddr);

    // only for nanos-lite,
    // the user process is mapped to 0x40000000 which is different.
    //Assert(paddr == vaddr, "paddr unequal vaddr!!!");

    return paddr;
}
