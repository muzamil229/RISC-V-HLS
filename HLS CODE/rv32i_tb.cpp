#include "rv32i.h"
#include <stdio.h>

int main() {
    arch_t mem[MEM_SIZE];
    
    // Initialize entire memory to 0.
    for (int i = 0; i < MEM_SIZE; i++) {
        mem[i] = 0;
    }
    
    // Preload data for load instructions:
    // mem[30] corresponds to address 30*4 = 120,
    // mem[31] corresponds to address 124.
  

    // Original instructions:
    // mem[0]: ADD R1, R2, R3 → R1 = 10 + 5 = 15
    mem[0] = 0x003100B3;
    // mem[1]: SUB R4, R2, R3 → R4 = 10 - 5 = 5
    mem[1] = 0x40310233;
    // mem[2]: BNE R2, R3, offset = 8 bytes → branch taken (since 10 ≠ 5)
    mem[2] = 0x00311463;
    // mem[3]: XOR R5, R2, R3 → should be skipped due to branch
    mem[3] = 0x003142B3;
    // mem[4]: JAL R7, offset = 8 bytes → R7 gets PC+4, jumps ahead
    mem[4] = 0x004003EF;
    // mem[5]: (unused, remains 0)
    // mem[6]: SLL R8, R2, R3 → R8 = 10 << 5 = 320
    mem[6] = 0x00311433;
    // mem[7]: SRL R9, R2, R3 → R9 = 10 >> 5 = 0
    mem[7] = 0x003154B3;
    // mem[8]: SW R1, 64(R0) → store R1 (15) to address 64 (i.e. mem[16])
    mem[8] = 0x04102023;
    // mem[9]: ADDI R10, R2, 3 → R10 = 10 + 3 = 13
    mem[9] = 0x00310513;
    // mem[10]: ORI R11, R2, 0xF0 → R11 = 10 | 0xF0 = 0xFA
    mem[10] = 0x0F016593;
    // mem[11]: ANDI R12, R2, 0x0F → R12 = 10 & 0x0F = 10
    mem[11] = 0x00F17613;
    // mem[12]: LUI R13, 0x12345 → R13 becomes 0x12345000
    mem[12] = 0x123456B7;
    // mem[13]: AUIPC R14, 0x10 → with PC = 52, R14 becomes 0x00010034
    mem[13] = 0x00010717;

    // Additional instructions:
    // mem[14]: XORI R15, R2, 5 → R15 = 10 ^ 5 = 15
    mem[14] = 0x00514793;
    // mem[15]: SLLI R16, R2, 2 → R16 = 10 << 2 = 40
    mem[15] = 0x00211813;
    // mem[16]: Will be overwritten by SW from mem[8] (R1=15 stored into address 64).
    
    // mem[17]: LB R18, 120(R0) → loads a byte from address 120.
    //  Encoding breakdown:
    //    Immediate = 0x078 (for address 120)
    //    rd = 18 → (18 << 7)
    //    opcode = 0x03, funct3 = 0x0 for LB.
    //  Final hex: 0x07802983.
    mem[17] = 0x01400903;
    
    // mem[18]: LW R19, 124(R0) → loads a word from address 124.
    //  Encoding breakdown:
    //    Immediate = 0x07C (for address 124)
    //    rd = 19 → (19 << 7)
    //    funct3 = 0x2 for LW, opcode = 0x03.
    //  Correct final hex: 0x07C02983.
    mem[18] = 0x01202983;
    
    // mem[19]: BLT R2, R3, offset = 8 bytes → direct hex 0x00314263.
    mem[19] = 0x00314263;
    // mem[20]: BGE R2, R3, offset = 8 bytes → direct hex 0x00315263.
    mem[20] = 0x00315263;
  
    // Run the CPU simulation.
    cpu(mem);
    
    // Dump memory contents to a file for verification.
    FILE *fp = fopen("mem_dump.txt", "w");
    if (fp) {
        for (int i = 0; i < MEM_SIZE; i++) {
            fprintf(fp, "mem[%d] = 0x%08x\n", i, (unsigned int) mem[i]);
        }
        fclose(fp);
    } else {
        printf("Error: Could not open mem_dump.txt for writing.\n");
    }
    
    return 0;
}
