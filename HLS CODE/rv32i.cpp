#include "rv32i.h"  
#include <stdio.h>

//---------------------------------------------------------------------
// Helper functions for encoding instructions (for reference only)
static inline arch_t R_type(ap_uint<7> funct7, ap_uint<5> rs2, ap_uint<5> rs1,
                            ap_uint<3> funct3, ap_uint<5> rd, ap_uint<7> opcode) {
    return (arch_t(funct7) << 25) | (arch_t(rs2) << 20) | (arch_t(rs1) << 15) |
           (arch_t(funct3) << 12) | (arch_t(rd) << 7) | opcode;
}

static inline arch_t J_type(ap_int<21> imm, ap_uint<5> rd, ap_uint<7> opcode) {
    arch_t insn = 0;
    insn |= ((imm[20] & 0x1) << 31);
    insn |= ((imm.range(19,12) & 0xFF) << 12);
    insn |= ((imm[11] & 0x1) << 20);
    insn |= ((imm.range(10,1) & 0x3FF) << 21);
    insn |= (arch_t(rd) << 7);
    insn |= opcode;
    return insn;
}

static inline arch_t B_type(ap_int<13> imm, ap_uint<5> rs2, ap_uint<5> rs1,
                            ap_uint<3> funct3, ap_uint<7> opcode) {
    arch_t insn = 0;
    insn |= ((imm[12] & 0x1) << 31);
    insn |= ((imm.range(10,5) & 0x3F) << 25);
    insn |= (arch_t(rs2) << 20);
    insn |= (arch_t(rs1) << 15);
    insn |= ((funct3 & 0x7) << 12);
    insn |= ((imm.range(4,1) & 0xF) << 8);
    insn |= ((imm[11] & 0x1) << 7);
    insn |= opcode;
    return insn;
}

static inline arch_t S_type(ap_int<12> imm, ap_uint<5> rs2, ap_uint<5> rs1,
                            ap_uint<3> funct3, ap_uint<7> opcode) {
    arch_t insn = 0;
    insn |= ((imm.range(11,5) & 0x7F) << 25);
    insn |= (arch_t(rs2) << 20);
    insn |= (arch_t(rs1) << 15);
    insn |= (arch_t(funct3) << 12);
    insn |= ((imm.range(4,0) & 0x1F) << 7);
    insn |= opcode;
    return insn;
}

//---------------------------------------------------------------------
// ALU: supports ADD, SUB, XOR, OR, AND, SLL, and SRL.
arch_t alu(arch_t op1, arch_t op2, ap_uint<3> funct3, ap_uint<7> funct7) {
    if (funct3 == 0x0) {
        if (funct7 == 0x00) return op1 + op2;  // ADD
        else if (funct7 == 0x20) return op1 - op2;  // SUB
    } else if (funct3 == 0x4 && funct7 == 0x00)
        return op1 ^ op2;   // XOR
    else if (funct3 == 0x6 && funct7 == 0x00)
        return op1 | op2;   // OR
    else if (funct3 == 0x7 && funct7 == 0x00)
        return op1 & op2;   // AND
    else if (funct3 == 0x1 && funct7 == 0x00)
        return op1 << op2;  // SLL
    else if (funct3 == 0x5 && funct7 == 0x00)
        return op1 >> op2;  // SRL
    return 0;
}

//---------------------------------------------------------------------
// Top-level CPU function for HLS (FSM-based, non-pipelined)
void cpu(arch_t mem[MEM_SIZE]) {
#pragma HLS INTERFACE ap_memory port=mem
#pragma HLS INTERFACE ap_ctrl_none port=return

    // Register file as a simple array (no partitioning)
    arch_t reg_file[REGFILE_SIZE];

    // Initialize registers (R0 remains 0)
    for (int i = 0; i < REGFILE_SIZE; i++) {
        reg_file[i] = 0;
    }
    reg_file[2] = 10; // R2 = 10
    reg_file[3] = 5;  // R3 = 5

    // Program counter (byte address) and temporary instruction storage
    arch_t pc = 0;
    arch_t insn = 0;

    // Define FSM states
    enum fsm_state { FETCH, DECODE, EXECUTE };
    fsm_state state = FETCH;

    // Temporary variables for decoding
    ap_uint<7> opcode = 0;
    ap_uint<5> rd = 0;
    ap_uint<3> funct3 = 0;
    ap_uint<5> rs1 = 0;
    ap_uint<5> rs2 = 0;
    ap_uint<7> funct7 = 0;

    // FSM-based processing loop
    while (pc < 128) {
        switch (state) {
            case FETCH:
                // Fetch the instruction from memory
                insn = mem[pc >> 2];
                state = DECODE;
                break;

            case DECODE:
                // Common decode: extract opcode and destination register
                opcode = insn.range(6,0);
                rd = insn.range(11,7);
                state = EXECUTE;
                break;

            case EXECUTE:
                // JAL Instruction
                if (opcode == 0x6F) {  
                    ap_int<21> imm_j;
                    imm_j.range(19,12) = insn.range(19,12);
                    imm_j[11]          = insn[20];
                    imm_j.range(10,1)  = insn.range(30,21);
                    imm_j[20]          = insn[31];
                    imm_j = imm_j << 1;  // Multiply offset by 2
                    if (rd != 0)
                        reg_file[rd] = pc + 4; // Save return address
                    pc = pc + imm_j;
                }
                // Branch Instructions
                else if (opcode == 0x63) {  
                    funct3 = insn.range(14,12);
                    rs1 = insn.range(19,15);
                    rs2 = insn.range(24,20);
                    ap_int<13> imm_b = ((static_cast<int>(insn[31]) & 0x1) << 12) |
                                       ((static_cast<int>(insn[7])  & 0x1) << 11) |
                                       ((static_cast<int>(insn.range(30,25))) << 5) |
                                       (static_cast<int>(insn.range(11,8)));
                    imm_b = imm_b << 1;
                    bool take_branch = false;
                    if (funct3 == 0x0) {  // BEQ
                        if (reg_file[rs1] == reg_file[rs2])
                            take_branch = true;
                    }
                    else if (funct3 == 0x1) {  // BNE
                        if (reg_file[rs1] != reg_file[rs2])
                            take_branch = true;
                    }
                    else if (funct3 == 0x4) {  // BLT (signed less than)
                        if ((int)reg_file[rs1] < (int)reg_file[rs2])
                            take_branch = true;
                    }
                    else if (funct3 == 0x5) {  // BGE (signed greater or equal)
                        if ((int)reg_file[rs1] >= (int)reg_file[rs2])
                            take_branch = true;
                    }
                    if (take_branch)
                        pc = pc + imm_b;
                    else
                        pc = pc + 4;
                }
                // Store Instructions
                else if (opcode == 0x23) {  
                    funct3 = insn.range(14,12);
                    if (funct3 == 0x2) { // SW
                        rs1 = insn.range(19,15);
                        rs2 = insn.range(24,20);
                        ap_int<12> imm_s = (insn.range(31,25) << 5) | insn.range(11,7);
                        arch_t addr = reg_file[rs1] + imm_s;
                        mem[addr >> 2] = reg_file[rs2];
                    }
                    pc = pc + 4;
                }
                // Load Instructions (LB, LW)
                else if (opcode == 0x03) {  
                    funct3 = insn.range(14,12);
                    rs1 = insn.range(19,15);
                    ap_int<12> imm = insn.range(31,20);
                    arch_t addr = reg_file[rs1] + imm;
                    arch_t loaded = 0;
                    if (funct3 == 0x0) {  // LB
                        int byte_index = addr & 0x3;
                        arch_t word = mem[addr >> 2];
                        char byte = (word >> (byte_index * 8)) & 0xFF;
                        loaded = (arch_t)((signed char)byte);
                    } else if (funct3 == 0x2) {  // LW
                        loaded = mem[addr >> 2];
                    }
                    if (rd != 0)
                        reg_file[rd] = loaded;
                    pc = pc + 4;
                }
                // I-Type Arithmetic Instructions (including XORI, SLLI, SRLI)
                else if (opcode == 0x13) {  
                    funct3 = insn.range(14,12);
                    rs1 = insn.range(19,15);
                    ap_int<12> imm = insn.range(31,20);
                    arch_t src1 = reg_file[rs1];
                    arch_t result = 0;
                    if (funct3 == 0x0)
                        result = src1 + imm;         // ADDI
                    else if (funct3 == 0x1)
                        result = src1 << (imm & 0x1F); // SLLI
                    else if (funct3 == 0x4)
                        result = src1 ^ imm;         // XORI
                    else if (funct3 == 0x5)
                        result = src1 >> (imm & 0x1F); // SRLI
                    else if (funct3 == 0x6)
                        result = src1 | imm;         // ORI
                    else if (funct3 == 0x7)
                        result = src1 & imm;         // ANDI
                    if (rd != 0)
                        reg_file[rd] = result;
                    pc = pc + 4;
                }
                // U-Type Instructions (LUI, AUIPC)
                else if (opcode == 0x37 || opcode == 0x17) {  
                    unsigned int insn_int = insn.to_uint();
                    unsigned int lower = (((unsigned int)rd) << 7) | ((unsigned int)opcode);
                    unsigned int imm_field = (insn_int - lower) >> 12;
                    if (opcode == 0x37) {  // LUI
                        arch_t result = imm_field << 12;
                        if (rd != 0)
                            reg_file[rd] = result;
                    }
                    else {  // AUIPC
                        arch_t result = (pc) + (imm_field << 12);
                        if (rd != 0)
                            reg_file[rd] = result;
                    }
                    pc = pc + 4;
                }
                // R-Type Arithmetic Instructions
                else if (opcode == 0x33) {  
                    funct3 = insn.range(14,12);
                    rs1 = insn.range(19,15);
                    rs2 = insn.range(24,20);
                    funct7 = insn.range(31,25);
                    arch_t src1 = reg_file[rs1];
                    arch_t src2 = reg_file[rs2];
                    arch_t result = alu(src1, src2, funct3, funct7);
                    if (rd != 0)
                        reg_file[rd] = result;
                    pc = pc + 4;
                }
                // ECALL Instruction
                else if (opcode == 0x73) {  
                    printf("Ecall encountered. Halting CPU.\n");
                    return;
                }
                // Unrecognized opcode: simply increment PC.
                else {  
                    pc = pc + 4;
                }
                state = FETCH;
                break;
        } // end switch(state)
    } // end while

    // Print register file contents for verification.
    for (int i = 0; i < REGFILE_SIZE; i++) {
        printf("R[%02d] = 0x%08x\n", i, (unsigned int) reg_file[i]);
    }
}
