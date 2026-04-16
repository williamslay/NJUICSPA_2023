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

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>
#define CSRNUMS 4

typedef enum {
    MSTATUS = 0x300,
    MTEVC = 0x305,
    MEPC = 0x341,
    MCAUSE = 0x342,
} CSRADDR;

typedef struct {
    CSRADDR csr;
    int idx;
} CSRAddr2Idx;

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

static inline int find_csr_idx(int addr)
{
  extern const CSRAddr2Idx CSRAddrmap[];
  for (int i = 0; i < CSRNUMS; i++) {
    if (CSRAddrmap[i].csr == addr) {
      return CSRAddrmap[i].idx;
    }
  }
  // should not reach here
  IFDEF(CONFIG_RT_CHECK, assert(0));
  return addr;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])
#define csrs(addr) (cpu.csrs[find_csr_idx(addr)])

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

static inline const char* csreg_name(int idx) {
  extern const char* csregs[];
  return csregs[idx];
}

#endif
