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
int debug = 0;      // Set to 1 for additional debugging information.
int pipe_trace = 1; // Set to 1 for pipe trace.
int pipe_trace_mode = 3;
int counter = 0;

/**
 * Fetch stage implementation.
 */
struct State fetch(struct State fetch_out)
{

    /**
     * TODO: Your logic for fetch stage here.
     */

    if (pipe_stall == 1)
    {
        return fetch_out;
    }

    // Use the current PC to store i) The instruction ii) The instruction address
    struct State fetch_out_temp = {0}; // Initialize to zero
    fetch_out_temp.inst = memory[pc / 4];
    fetch_out_temp.inst_addr = pc;

    // Advance the (next) PC
    advance_pc(fetch_out_temp.inst_addr + 4);

    // Return the instruction
    return fetch_out_temp;
}

/**
 * Decode stage implementation
 */
struct State decode(struct State fetch_out)
{

    /**
     * TODO: Your code for the decode stage here.
     */

    // initiate the read enable registers
    int re_reg1 = 0;
    int re_reg2 = 0;


    if (fetch_out.opcode == RTYPE || fetch_out.opcode == STYPE)
    {
        re_reg1 = 1;
        re_reg2 = 1;
    }
    else if (fetch_out.opcode == ITYPE_LOAD || fetch_out.opcode == ITYPE_ARITH)
    {
        re_reg1 = 1;
        re_reg2 = 0;
    } else{
        re_reg1 = 0;
        re_reg2 = 0;
    }

    // Decode stage
    // printf("******** Decoding:  *********\n");

    // Check for hazards on rs1
    if ((fetch_out.rs1 != 0 && re_reg1) && ((we_exe && fetch_out.rs1 == ws_exe) || // Hazard with execute stage
                                 (we_mem && fetch_out.rs1 == ws_mem) || // Hazard with memory stage
                                 (we_wb && fetch_out.rs1 == ws_wb)))
    { // Hazard with writeback stage

        pipe_stall = 1;
        return nop; // Stall pipeline with a NOP
    } 
    // Check for hazards on rs2
    if ((fetch_out.rs2 != 0 && re_reg2) && ((we_exe && fetch_out.rs2 == ws_exe) ||
                                 (we_mem && fetch_out.rs2 == ws_mem) ||
                                 (we_wb && fetch_out.rs2 == ws_wb)))
    {

        pipe_stall = 1;
        return nop; // Stall pipeline with a NOP
    } 

    decode_fields(&fetch_out);

    // printf("Instruction: %x\n", fetch_out.inst);
    // printf("Instruction Address: %x\n", fetch_out.inst_addr);
    // printf("Opcode: %x\n", fetch_out.opcode);
    // printf("Funct3: %x\n", fetch_out.funct3);
    // printf("Funct7: %x\n", fetch_out.funct7);
    // printf("RD: %x\n", fetch_out.rd);
    // printf("RS1: %x\n", fetch_out.rs1);
    // printf("RS2: %x\n", fetch_out.rs2);
    // printf("Immediate: %x\n", fetch_out.imm);

    // if (fetch_out.opcode == LUI) // U-Type Instructions: LUI
    //     fetch_out.imm = fetch_out.inst & bit_31_downto_12;



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
    // case LUI: // I-Type Instructions - opcode = 0x13 or 0x3
        fetch_out.alu_in1 = registers[fetch_out.rs1];
        fetch_out.alu_in2 = fetch_out.imm;

        if (fetch_out.opcode == STYPE)
        {
            fetch_out.mem_buffer = registers[fetch_out.rs2];
        }

        break;

    default:
        break;
    }

    

    return fetch_out;
}

/**
 * Execute stage implementation
 */
struct State execute(struct State decode_out)
{

    /**
     * TODO: Your code for the decode stage here.
     */
    we_exe = 0;
    ws_exe = 0; 

    switch (decode_out.opcode)
    {
    case RTYPE:
        // R-Type
        if (decode_out.funct7 == ADD_F7)
        {
            decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
        }
        else if (decode_out.funct7 == SUB_F7)
        {
            decode_out.alu_out = decode_out.alu_in1 - decode_out.alu_in2;
        }
        else if (decode_out.funct3 == SLT)
        {
            decode_out.alu_out = (decode_out.alu_in1 < decode_out.alu_in2) ? 1 : 0;
        }
        else if (decode_out.funct3 == SLL)
        {
            decode_out.alu_out = decode_out.alu_in1 << decode_out.alu_in2;
        }
        else if (decode_out.funct3 == SRL)
        {
            decode_out.alu_out = decode_out.alu_in1 >> decode_out.alu_in2;
        }
        else if (decode_out.funct3 == AND)
        {
            decode_out.alu_out = decode_out.alu_in1 & decode_out.alu_in2;
        }
        else if (decode_out.funct3 == OR)
        {
            decode_out.alu_out = decode_out.alu_in1 | decode_out.alu_in2;
        }
        else if (decode_out.funct3 == XOR)
        {
            decode_out.alu_out = decode_out.alu_in1 ^ decode_out.alu_in2;
        } 
        we_exe = 1;
        ws_exe = decode_out.rd;
        break;

    case ITYPE_LOAD:
        if (decode_out.funct3 == LW_SW)
        {
            decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
            decode_out.mem_addr = decode_out.alu_out;
        }
        we_exe = 1;
        ws_exe = decode_out.rd;
        break;

    case ITYPE_ARITH:
        // I-Type Instructions:  LW, ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI
        if (decode_out.funct3 == ADD_SUB)
        { // check with TA here
            decode_out.alu_out = decode_out.alu_in1 + decode_out.imm;
        }
        else if (decode_out.funct3 == AND)
        { // ANDI
            decode_out.alu_out = decode_out.alu_in1 & decode_out.imm;
        }
        else if (decode_out.funct3 == OR)
        { // ORI
            decode_out.alu_out = decode_out.alu_in1 | decode_out.imm;
        }
        else if (decode_out.funct3 == XOR)
        {                                                             // XOR
            decode_out.alu_out = decode_out.alu_in1 ^ decode_out.imm; // check this again
        }
        else if (decode_out.funct3 == SLT)
        { // SLTI
            decode_out.alu_out = (decode_out.alu_in1 < decode_out.imm) ? 1 : 0;
        }
        else if (decode_out.funct3 == SLL)
        { // SLLI
            decode_out.alu_out = decode_out.alu_in1 << decode_out.imm;
        }
        else if (decode_out.funct3 == SRL)
        { // SRLI
            decode_out.alu_out = decode_out.alu_in1 >> decode_out.imm;
        } 


        we_exe = 1;
        ws_exe = decode_out.rd;
        break;

    case STYPE:
        if (decode_out.funct3 == LW_SW)
        {
            // counter++;
            // printf("Counter: %d\n", counter);
            
            decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
            decode_out.mem_addr = decode_out.alu_out;

        //     printf("AlU In1: %x\n", decode_out.alu_in1);
        //     printf("Immediate ALU In2: %d\n", decode_out.alu_in2);
        //     printf("Memory Address: %x\n", decode_out.alu_out);
        //     printf("Memory Value: %x\n", memory[decode_out.alu_out]);
        }
        we_exe = 0;
        ws_exe = 0;

        break;

    case LUI:
        // decode_out.alu_out = decode_out.imm;
        // The upper 20 bits of the ALU output is set to the immediate value. The lower 12 bits
        // of the ALU are set to 0
        decode_out.imm = decode_out.inst & bit_31_downto_12;
        decode_out.alu_out = decode_out.imm;
        

        we_exe = 1;
        ws_exe = decode_out.rd;

        break;

    default:
        break;
    }

    if (decode_out.opcode == RTYPE || decode_out.opcode == ITYPE_ARITH)
    {
        we_exe = 1;
        ws_exe = decode_out.rd;
    }
    else
    {
        we_exe = 0;
    }

    // execute the instruction

    return decode_out;
}

/**
 * Memory stage implementation
 */
struct State memory_stage(struct State ex_out)
{

    /**
     * TODO: Your code for the memory stage here.
     */

    ws_mem = 0;
    we_mem = 0;

    // Memory stage
    if (ex_out.opcode == ITYPE_LOAD && ex_out.funct3 == LW_SW)
    { // Load Word
        ex_out.mem_buffer = memory[ex_out.mem_addr];
        we_mem = 1;
        ws_mem = ex_out.rd;
    }
    else if (ex_out.opcode == STYPE && ex_out.funct3 == LW_SW)
    { // Store Word
        // memory[ex_out.alu_out] = ex_out.rs2;
        memory[ex_out.mem_addr] = ex_out.mem_buffer;
         ws_mem = 0;
         we_mem = 0;
    }
    

    return ex_out;
}

/**
 * Writeback stage implementation
 */
unsigned int writeback(struct State mem_out)
{

    /**
     * TODO: Your code for the write back stage here.
     */

    we_wb = 0;
    ws_wb = 0;

    // Writeback stage
    if (mem_out.opcode == RTYPE || mem_out.opcode == ITYPE_ARITH)
    {
        registers[mem_out.rd] = mem_out.alu_out;
        we_wb = 1;
        ws_wb = mem_out.rd;
    }
    else if (mem_out.opcode == ITYPE_LOAD && mem_out.funct3 == LW_SW)
    {
        registers[mem_out.rd] = mem_out.mem_buffer;
        we_wb = 1;
        ws_wb = mem_out.rd;
    }
    else if (mem_out.opcode == LUI)
    {
        // registers[mem_out.rd] = mem_out.alu_out;
        // registers[mem_out.rd] = mem_out.imm << 12;
        registers[mem_out.rd] = mem_out.alu_out;
        we_wb = 1;
        ws_wb = mem_out.rd;
    }
   

    if(pipe_stall==1){
        pipe_stall=0;
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