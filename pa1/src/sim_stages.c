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

   if (pipe_stall == 1){
    return fetch_out;
   } 

    //Use the current PC to store i) The instruction ii) The instruction address
    struct State fetch_out_temp = {0};  // Initialize to zero
    fetch_out_temp.inst = memory[pc / 4];
    fetch_out_temp.inst_addr = pc;

    // pipe_stall = 1;



    //Advance the (next) PC
    advance_pc(fetch_out.inst_addr + 4);


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

    // Decode stage
    printf("******** Decoding:  *********\n");




   
    decode_fields(&fetch_out);

    printf("Instruction: %x\n", fetch_out.inst);
    printf("Instruction Address: %x\n", fetch_out.inst_addr);
    printf("Opcode: %x\n", fetch_out.opcode);
    printf("Funct3: %x\n", fetch_out.funct3);
    printf("Funct7: %x\n", fetch_out.funct7);
    printf("RD: %x\n", fetch_out.rd);
    printf("RS1: %x\n", fetch_out.rs1);
    printf("RS2: %x\n", fetch_out.rs2);
    printf("Immediate: %x\n", fetch_out.imm);

    if (fetch_out.opcode == LUI) // U-Type Instructions: LUI
        fetch_out.imm = fetch_out.inst & bit_31_downto_12; 



    // determine the instruction type based on the opcode
    switch (fetch_out.opcode)
    {
        case RTYPE: // R-Type Instructions - opcode = 0x33
            fetch_out.alu_in1 = registers[fetch_out.rs1];
            fetch_out.alu_in2 = registers[fetch_out.rs2];
            break;
            
        case ITYPE_LOAD:
        case ITYPE_ARITH:
        case STYPE:
        case LUI: // I-Type Instructions - opcode = 0x13 or 0x3
            fetch_out.alu_in1 = registers[fetch_out.rs1];
            fetch_out.alu_in2 = fetch_out.imm;
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

   switch (decode_out.opcode)
   {
   case RTYPE:
       // R-Type
        if (decode_out.funct7 == ADD_F7) {
            decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
        } else if (decode_out.funct7 == SUB_F7) {
            decode_out.alu_out = decode_out.alu_in1 - decode_out.alu_in2;
        } else if (decode_out.funct3 == SLT) {
            decode_out.alu_out = (decode_out.alu_in1 < decode_out.alu_in2) ? 1 : 0;
        } else if (decode_out.funct3 == SLL) {
            decode_out.alu_out = decode_out.alu_in1 << decode_out.alu_in2;
        } else if (decode_out.funct3 == SRL) {
            decode_out.alu_out = decode_out.alu_in1 >> decode_out.alu_in2;
        } else if (decode_out.funct3 == AND) {
            decode_out.alu_out = decode_out.alu_in1 & decode_out.alu_in2;
        } else if (decode_out.funct3 == OR) {
            decode_out.alu_out = decode_out.alu_in1 | decode_out.alu_in2;
        } else if (decode_out.funct3 == XOR) {
            decode_out.alu_out = decode_out.alu_in1 ^ decode_out.alu_in2;
        } 
        break;

    case ITYPE_LOAD:
  if (decode_out.funct3 == LW_SW){
            decode_out.alu_out = decode_out.alu_in1 + decode_out.imm;
            decode_out.rd = memory[decode_out.alu_out];
            decode_out.mem_addr = decode_out.alu_out;
        } else 

    case ITYPE_ARITH:
        // I-Type Instructions:  LW, ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI
      if (decode_out.funct3 == ADD_SUB){ // check with TA here
            decode_out.alu_out = decode_out.alu_in1 + decode_out.imm;
        } else if (decode_out.funct3 == AND){ // ANDI
            decode_out.alu_out = decode_out.alu_in1 & decode_out.imm;
        } else if (decode_out.funct3 == OR){ // ORI
                decode_out.alu_out = decode_out.alu_in1 | decode_out.imm;
        } else if (decode_out.funct3 == XOR){ // XOR
                decode_out.alu_out = decode_out.alu_in1 ^ decode_out.alu_in2;
        } else if (decode_out.funct3 == SLT) { // SLTI
                decode_out.alu_out = (decode_out.alu_in1 < decode_out.imm) ? 1 : 0;
        } else if (decode_out.funct3 == SLL) { // SLLI
            decode_out.alu_out = decode_out.alu_in1 << decode_out.imm;
        } else if (decode_out.funct3 == SRL) { // SRLI
            decode_out.alu_out = decode_out.alu_in1 >> decode_out.imm;
        } 
        break;

    case STYPE:
        if (decode_out.funct3 == LW_SW){
                decode_out.alu_out = decode_out.alu_in1 + decode_out.imm;
                memory[decode_out.alu_out] = decode_out.alu_in2;
            } 

        break;

    case LUI:
        decode_out.rd = decode_out.imm;
    // The upper 20 bits of the ALU output is set to the immediate value. The lower 12 bits
// of the ALU are set to 0
        // decode_out.alu_out = decode_out.imm << 12;
        break;
   
   default:
    break;

   }
   // execute the instruction

    return decode_out;
}

/**
 * Memory stage implementation
 */
struct State memory_stage(struct State ex_out) {
    
    /**
     * TODO: Your code for the memory stage here.
    */

   // Memory stage
    if (ex_out.opcode == ITYPE_LOAD && ex_out.funct3 == LW_SW) { // Load Word
        ex_out.mem_buffer = memory[ex_out.mem_addr];
    } else if (ex_out.opcode == STYPE && ex_out.funct3 == LW_SW) { // Store Word
        // memory[ex_out.alu_out] = ex_out.rs2;
        memory[ex_out.mem_addr] = ex_out.mem_buffer;
        

    } else {

    }






    return ex_out;
}

/**
 * Writeback stage implementation
 */
unsigned int writeback(struct State mem_out) {

    /**
     * TODO: Your code for the write back stage here.
    */

    // Writeback stage
    if (mem_out.opcode == RTYPE || mem_out.opcode == ITYPE_ARITH) {
        registers[mem_out.rd] = mem_out.alu_out;
    } else if (mem_out.opcode == ITYPE_LOAD && mem_out.funct3 == LW_SW) {
        registers[mem_out.rd] = mem_out.mem_buffer;
    } else if (mem_out.opcode == LUI) {
        // registers[mem_out.rd] = mem_out.alu_out;
        registers[mem_out.rd] = mem_out.imm << 12; 
    } else {
    }

    return mem_out.inst;
}


/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}