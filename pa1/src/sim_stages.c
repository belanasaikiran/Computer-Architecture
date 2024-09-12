/**
 * University of Connecticut
 * CSE 4302: Computer Architecture
 * Fall 2024
 * riscy-uconn: sim_stages.c
 * 
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "instruction_map.h"
#include "sim_core.h"
#include "sim_stages.h"
#include "decode_fields.h"

/**
 * Debug flags
 */
int debug = 0;          // Set to 1 for additional debugging information.
int pipe_trace = 1;     // Set to 1 for pipe trace.
int pipe_trace_mode = 3;


/**
 * Fetch stage implementation.
 */
struct State fetch(struct State fetch_out) {

    /**
     * TODO: Your logic for fetch stage here.
     */

    unsigned int inst;
    if (pc == 0) {
        inst = 0; // Set instruction to 0 when pc is 0
    } else {
        inst = memory[pc / 4];
    }


    // R-Type Instructions: ADD, SUB, AND, OR, XOR, SLT, SLL, SRL
    unsigned int inst_addr = pc;
    unsigned int opcode = inst & bit_6_downto_0;
    unsigned int rd = (inst >> 7) & bit_4_downto_0; // 0x1F = 0001 1111 - has 5 bits set to 1
    unsigned int funct3 = (inst >> 12) & bit_2_downto_0; // 0x7 = 0000 0111 - has 3 bits set to 1
    unsigned int rs1 = (inst >> 15) & bit_4_downto_0; // 0x1F = 0001 1111 - has 5 bits set to 1
    unsigned int rs2 = (inst >> 20) & bit_4_downto_0; // 0x1F = 0001 1111 - has 5 bits set to 1
    unsigned int funct7 = (inst >> 25) & bit_6_downto_0; // 0x7F = 0111 1111 - has 7 bits set to 1
    
    
    unsigned int imm = 0;

    // I-Type Instructions:  LW, ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI
    if (opcode == ITYPE_ARITH || ITYPE_LOAD)
        imm = (signed)(inst & bit_31_downto_20) >> 20; // 0xFFF0000 = 1111 1111 1111 0000 0000 0000 0000 0000 - has 12 bits set to 1
    else if (opcode == STYPE) // S-Type Instructions: SW
        imm = ((signed)(inst & bit_31_downto_25) >> 20) | ((inst & bit_11_downto_7) >> 7); // Right shift 20 bits and OR with the offset bits 31-25 and 11-7. We do right shift 20 bits beacuse the remaining 5 bits are for 11-7 offset bits.
    else if (opcode == LUI) // U-Type Instructions: LUI
        imm = inst & bit_31_downto_12; 
    else 
        imm = 0; // Set immediate to 0 for all other instructions
    
    

    printf("Instruction: %x\n", inst);
    printf("Instruction Address: %x\n", inst_addr);
    printf("Opcode: %x\n", opcode);
    printf("Funct3: %x\n", funct3);
    printf("Funct7: %x\n", funct7);
    printf("RD: %x\n", rd);
    printf("RS1: %x\n", rs1);
    printf("RS2: %x\n", rs2);
    printf("Immediate: %x\n", imm);



    //Advance the (next) PC
    advance_pc(inst_addr + 4);

    //Use the current PC to store i) The instruction ii) The instruction address
    struct State fetch_out_temp = {inst, inst_addr, opcode, funct3, funct7, rd, rs1, rs2, imm};

    //Return the instruction
    return fetch_out_temp;
}

/**
 * Decode stage implementation
 */
struct State decode(struct State fetch_out) {
    
    /**
     * TODO: Your code for the decode stage here.
    */

   decode_fields(&fetch_out);
    // determine the instruction type based on the opcode
    switch (fetch_out.opcode)
    {
    
    case RTYPE: // R-Type Instructions - opcode = 0x33
        fetch_out.alu_in1 = registers[fetch_out.rs1];
        fetch_out.alu_in2 = registers[fetch_out.rs2];
        break;
        
    case ITYPE_LOAD || ITYPE_ARITH: // I-Type Instructions - opcode = 0x13 or 0x3
        fetch_out.alu_in1 = registers[fetch_out.rs1];
        fetch_out.alu_in2 = fetch_out.imm;
    
        break;

    case 0x23: // S-Type Instructions - opcode = 0x23
        fetch_out.alu_in1 = registers[fetch_out.rs1];



        break;

    case 0x37: // U-Type Instructions - opcode = 0x37
        fetch_out.alu_in1 = 0;
        fetch_out.alu_in2 = fetch_out.inst & 0xFFFFF000; 

        break;

    default:
        break;
    }    
    
    
    return fetch_out; 
}

/**
 * Execute stage implementation
 */
struct State execute(struct State decode_out) {

    /**
     * TODO: Your code for the decode stage here.
    */

   // execute the instruction
    printf("Executing instruction\n");



   

    return decode_out;
}

/**
 * Memory stage implementation
 */
struct State memory_stage(struct State ex_out) {
    
    /**
     * TODO: Your code for the decode stage here.
    */



    return ex_out;
}

/**
 * Writeback stage implementation
 */
unsigned int writeback(struct State mem_out) {

    /**
     * TODO: Your code for the decode stage here.
    */

    return mem_out.inst;
}


/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}