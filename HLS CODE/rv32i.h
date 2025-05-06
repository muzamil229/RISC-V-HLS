#ifndef RV32I_H
#define RV32I_H

#include "ap_int.h"

// Memory and register file sizes
#define MEM_SIZE     1024
#define REGFILE_SIZE 32

// 32-bit word type for our architecture
typedef ap_uint<32> arch_t;

// Function prototypes
void cpu(arch_t mem[MEM_SIZE]);
arch_t alu(arch_t op1, arch_t op2, ap_uint<3> funct3, ap_uint<7> funct7);

#endif // RV32I_H
