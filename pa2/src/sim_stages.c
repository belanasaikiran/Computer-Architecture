/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 *
 *
 * riscy-uconn: sim_stages.c
 *
 * DO NOT MODIFY THIS FILE
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
int debug = 0; // Set to 1 for additional debugging information.
int pipe_trace = 1; // Set to 1 for pipe trace.
int pipe_trace_mode = 3;

/**
 * Fetch stage implementation.
 */
struct State fetch(struct State fetch_out) {

  /**
   * TODO: Your logic for fetch stage here.
   */

  if (pipe_stall == 1) {
    return fetch_out;
  }
// Handling control hazards: return nop if jump or branch is taken
  if (j_taken == 1) {
    j_taken = 0; // reset the j_taken flag
    return nop; // inserting a bubble
  }

  if(br_mispredicted == 1){
    br_mispredicted = 0; // reset the br_mispredicted flag
    return nop; // inserting a bubble
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
struct State decode(struct State fetch_out) {

  /**
   * TODO: Your code for the decode stage here.
   */

    pipe_stall = 0; // reset the pipe_stall flag
      //branch misprediction - insert a bubble
    if (br_mispredicted == 1) {
    // br_mispredicted = 0;
        return nop;
    }
    // j_taken = 0;
    // br_mispredicted = 0;
       // initiate the read enable registers
    int re_reg1 = 0;
    int re_reg2 = 0;

    decode_fields( & fetch_out);

    // Interlock logic
    if (fetch_out.opcode == RTYPE || fetch_out.opcode == STYPE || fetch_out.opcode == BTYPE) {
        re_reg1 = 1;
        re_reg2 = 1;
    } else if (fetch_out.opcode == ITYPE_LOAD || fetch_out.opcode == ITYPE_ARITH) {
        re_reg1 = 1;
    }

    // Check for forwarding enabled
    if (forwarding_enabled) {
        if((fetch_out.rs1 != 0) && re_reg1 == 1){
            if(we_exe && fetch_out.rs1 == ws_exe){
                registers[fetch_out.rs1] = dout_exe;
            } else if(we_mem && fetch_out.rs1 == ws_mem){
                registers[fetch_out.rs1] = dout_mem;
            } else if(we_wb && fetch_out.rs1 == ws_wb){
                registers[fetch_out.rs1] = dout_wb;
            }

            fetch_out.alu_in1 = registers[fetch_out.rs1];
        }
      
        if((fetch_out.rs2 != 0) && re_reg2 == 1){
            if(we_exe && fetch_out.rs2 == ws_exe){
                registers[fetch_out.rs2] = dout_exe;
            } else if(we_mem && fetch_out.rs2 == ws_mem){
                registers[fetch_out.rs2]  = dout_mem;
            } else if(we_wb && fetch_out.rs2 == ws_wb){
                registers[fetch_out.rs2]  = dout_wb;
            }
            fetch_out.alu_in2 = registers[fetch_out.rs2];
        }
    }else{
    // Interlock check
    // Check for hazards on rs1 and rs2
        if ((((fetch_out.rs1 != 0) && re_reg1) && ((we_exe && (fetch_out.rs1 == ws_exe)) || // Hazard with execute stage
                (we_mem && (fetch_out.rs1 == ws_mem)) || // Hazard with memory stage
                (we_wb && (fetch_out.rs1 == ws_wb)))) || // Hazard with writeback stage
                (((fetch_out.rs2 != 0) && re_reg2) && ((we_exe && (fetch_out.rs2 == ws_exe)) || // Hazard with execute stage
                (we_mem && (fetch_out.rs2 == ws_mem)) || // Hazard with memory stage
                (we_wb && (fetch_out.rs2 == ws_wb))))) { // Hazard with writeback stage
            pipe_stall = 1;
            return nop; // Stall pipeline with a NOP
        }
    }


    // resolve JAL and JALR instructions
    // if(j_taken == 1){
    //     j_taken = 0;
    // }

    if (lw_in_exe == 1) {
    pipe_stall = 1;
    lw_in_exe = 0;
    return nop;
    }

    // Sign extension
    // int32_t imm_signed = (int32_t)(fetch_out.imm << 20) >> 20; // 12-bit sign extension

    // determine the instruction type based on the opcode
    switch (fetch_out.opcode) {
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

            if (fetch_out.opcode == STYPE) {
            fetch_out.mem_buffer = registers[fetch_out.rs2];
            }

            break;

        case LUI:
            // fill this - might need to fix this
            fetch_out.imm = fetch_out.inst & bit_31_downto_12; // last 12 bits set to 0
            break;


        case JAL:
            fetch_out.link_addr = fetch_out.inst_addr + 4;
            pc_n = fetch_out.inst_addr + fetch_out.imm;
            j_taken = 1;
        break;
        case JALR:
            fetch_out.link_addr = fetch_out.inst_addr + 4;
            pc_n = registers[fetch_out.rs1] + fetch_out.imm;
            j_taken = 1;
            break;

        case BTYPE:
            /* registers[rs1] and registers[rs2] are read as the two ALU operands. br_addr is
            set to inst_addr + immediate. */
            fetch_out.alu_in1 = registers[fetch_out.rs1];
            fetch_out.alu_in2 = registers[fetch_out.rs2];
            fetch_out.br_addr = fetch_out.inst_addr + fetch_out.imm;
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

  we_exe = 0;
  ws_exe = 0;
  lw_in_exe = 0;
  dout_exe = 0;
  br_mispredicted = 0;


  // Do a check if nop is passed and return the same
  if (decode_out.inst == nop.inst) {
    return decode_out;
  }

// // reset br_mispredicted flag
// if(br_mispredicted){
//     br_mispredicted = 0;
// }

  // use Switch case for setting ws_exe and we_exe

  switch (decode_out.opcode) {
  case RTYPE:
    // R-Type
    if (decode_out.funct3 == ADD_SUB) {
      if(decode_out.funct7 == SUB_F7){
        decode_out.alu_out = decode_out.alu_in1 - decode_out.alu_in2;
      } else {
        decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
      }
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
    } else {}
    we_exe = 1;
  ws_exe = decode_out.rd;
    dout_exe = decode_out.alu_out;
    break;

  case ITYPE_LOAD:
    if (decode_out.funct3 == LW_SW) {
      decode_out.mem_addr = decode_out.alu_in1 + decode_out.alu_in2;
    }
    we_exe = 1;
    dout_exe = decode_out.alu_out;
  ws_exe = decode_out.rd;
    lw_in_exe = 1;
    break;

  case ITYPE_ARITH:
    // I-Type Instructions:  LW, ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI
    if (decode_out.funct3 == ADD_SUB) { // check with TA here
      decode_out.alu_out = decode_out.alu_in1 + decode_out.imm;
    } else if (decode_out.funct3 == AND) { // ANDI
      decode_out.alu_out = decode_out.alu_in1 & decode_out.imm;
    } else if (decode_out.funct3 == OR) { // ORI
      decode_out.alu_out = decode_out.alu_in1 | decode_out.imm;
    } else if (decode_out.funct3 == XOR) { // XOR
      decode_out.alu_out = decode_out.alu_in1 ^ decode_out.imm; // check this again
    } else if (decode_out.funct3 == SLT) { // SLTI
      decode_out.alu_out = (decode_out.alu_in1 < decode_out.imm) ? 1 : 0;
    } else if (decode_out.funct3 == SLL) { // SLLI
      decode_out.alu_out = decode_out.alu_in1 << decode_out.imm;
    } else if (decode_out.funct3 == SRL) { // SRLI
      decode_out.alu_out = decode_out.alu_in1 >> decode_out.imm;
    } else {}

    we_exe = 1;
    ws_exe = decode_out.rd;
    dout_exe = decode_out.alu_out;
    // ws_exe = decode_out.rd;
    break;

  case STYPE:
    decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
    decode_out.mem_addr = decode_out.alu_out;


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

  case BTYPE:
     if ((decode_out.funct3 == BEQ && decode_out.alu_in1 == decode_out.alu_in2) ||
        (decode_out.funct3 == BNE && decode_out.alu_in1 != decode_out.alu_in2) ||
        (decode_out.funct3 == BLT && (int32_t)decode_out.alu_in1 < (int32_t)decode_out.alu_in2) ||
        (decode_out.funct3 == BGE && (int32_t)decode_out.alu_in1 >= (int32_t)decode_out.alu_in2)) {
        br_mispredicted = 1;
        pc_n = decode_out.br_addr;
    } else{
        br_mispredicted = 0;
    }

    //   we_exe = 0;


    break;
    case JAL:
    case JALR:
        // dout_exe = decode_out.link_addr;
        // pc_n = decode_out.br_addr;
        // we_exe = 1;
        break;


  default:
    break;
  }


  return decode_out;
}

/**
 * Memory stage implementation
 */
struct State memory_stage(struct State ex_out) {

  /**
   * TODO: Your code for the decode stage here.
   */
  we_mem = we_exe;
  ws_mem = ws_exe;
  dout_mem = 0;

  // Memory stage
  if (ex_out.opcode == ITYPE_LOAD) { // Load Word
    ex_out.mem_buffer = memory[ex_out.mem_addr];
    dout_mem = ex_out.mem_buffer;
  } else if (ex_out.opcode == STYPE) { // Store Word
    memory[ex_out.mem_addr] = ex_out.mem_buffer;
  }

  if(ex_out.opcode == RTYPE || ex_out.opcode == ITYPE_ARITH || ex_out.opcode == LUI){
    dout_mem = ex_out.alu_out;
  }

  return ex_out;
}

/**
 * Writeback stage implementation
 */
unsigned int writeback(struct State mem_out) {

  /**
   * TODO: Your code for the decode stage here.
   */
  we_wb = we_mem;
  ws_wb = ws_mem;
  dout_wb = 0;

  // Writeback stage
  if (mem_out.opcode == RTYPE || mem_out.opcode == ITYPE_ARITH || mem_out.opcode == LUI) {
    registers[mem_out.rd] = mem_out.alu_out;
    dout_wb = mem_out.alu_out;
  } else if (mem_out.opcode == ITYPE_LOAD) {
    registers[mem_out.rd] = mem_out.mem_buffer;
    dout_wb = mem_out.mem_buffer;
  } else if (mem_out.opcode == JAL || mem_out.opcode == JALR) {
    if(mem_out.rd != 0){
        registers[mem_out.rd] = mem_out.link_addr;
        // dout_wb = mem_out.link_addr;
    }
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