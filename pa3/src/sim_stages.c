/**
 * University of Connecticut
 * CSE 4302: Computer Architecture
 * Fall 2024
 * 
 * Programming Assignment 3: MultiCycle Stall + In order + Seperate Execution Units
 * 
 * riscy-uconn: sim_stages.c
 * 
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_map.h"
#include "sim_core.h"
#include "sim_stages.h"
#include "decode_fields.h"

/**
 * Debug flags
 */
int debug = 0;           /* Set to 1 for additional debugging information. */
int pipe_trace = 1;      /* Set to 1 for pipe trace. */
int pipe_trace_mode = 3; /* See PA1 handout, section 5 for usage */




/**
 * Fetch stage implementation. 
 */
struct State fetch() {
    
    if (debug) printf("Stage: FETCH, dmem_busy: %d, pipe_stall: %d, inst: %d\n", dmem_busy, pipe_stall, fetch_out.inst);

    // Stall or Flush handling
    if (dmem_busy == 1 || pipe_stall == 1) return fetch_out; // stall fetch when memory (LD/SW) is busy,Return current state if stalled

    // Handling control hazards: return nop if jump or branch is taken
    if (j_taken == 1 || br_mispredicted == 1) return nop; // inserting a bubble

    fetch_out_n = fetch_out; // Copy the current fetch state to the next state

    // Fetch new instruction
    fetch_out_n.inst = memory[pc / 4];
    fetch_out_n.inst_addr = pc;

    // Advance the (next) PC
    advance_pc(fetch_out_n.inst_addr + 4);

    // Return the next fetch state
    return fetch_out_n;

}

/**
 * Decode stage implementation
 */
struct State decode() {

   
    if(dmem_busy == 1) return nop; // stall decode when memory (LD/SW) is busy
    if (br_mispredicted == 1) return nop; //branch misprediction - insert a bubble

    // Reset the pipe_stall flag if there are no hazards
    if (pipe_stall == 1) {
        // Check if the conditions that caused the stall are cleared
        if (!(we_exe && (decode_out_n.rs1 == ws_exe || decode_out_n.rs2 == ws_exe)) &&
            !(we_mem && (decode_out_n.rs1 == ws_mem || decode_out_n.rs2 == ws_mem)) &&
            !(we_wb && (decode_out_n.rs1 == ws_wb || decode_out_n.rs2 == ws_wb))) {
            pipe_stall = 0;  // Reset the stall if no hazards
        }
    }

    if (pipe_stall == 1) return nop; 

    // Copy fetch_out to decode_out_n
    decode_out_n = fetch_out;

    // reset
    pipe_stall = 0; // reset the pipe_stall flag
    j_taken = 0;
    br_mispredicted = 0;
    
    // initiate the read enable registers
    int re_reg1 = 0;
    int re_reg2 = 0;

    decode_fields( &decode_out_n);

    // Interlock logic
    if (decode_out_n.opcode == RTYPE || decode_out_n.opcode == STYPE || decode_out_n.opcode == BTYPE) {
        re_reg1 = 1;
        re_reg2 = 1;
    } else if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == ITYPE_ARITH || decode_out_n.opcode == JALR) {
        re_reg1 = 1;
    }

    // Check for forwarding enabled
    if (forwarding_enabled) {
        if((decode_out_n.rs1 != 0) && re_reg1 == 1){

            // stall the pipeline if there is a hazard
            if(we_exe && decode_out_n.rs1 == ws_exe){
                registers[decode_out_n.rs1] = dout_exe;
                if (dmem_busy == 1) { // changed from lw_in_exe to dmem_busy
                    pipe_stall = 1;
                    return nop;
                }
            } else if(we_mem && decode_out_n.rs1 == ws_mem){
                registers[decode_out_n.rs1] = dout_mem;
            } else if(we_wb && decode_out_n.rs1 == ws_wb){
                registers[decode_out_n.rs1] = dout_wb;
            }

            decode_out_n.alu_in1 = registers[decode_out_n.rs1];
        }
      
        if((decode_out_n.rs2 != 0) && re_reg2 == 1){
            if(we_exe && decode_out_n.rs2 == ws_exe){
                registers[decode_out_n.rs2] = dout_exe;
                if (dmem_busy == 1) {
                  pipe_stall = 1;
                  return nop;
                  }
            } else if(we_mem && decode_out_n.rs2 == ws_mem){
                registers[decode_out_n.rs2]  = dout_mem;
            } else if(we_wb && decode_out_n.rs2 == ws_wb){
                registers[decode_out_n.rs2]  = dout_wb;
            }
            decode_out_n.alu_in2 = registers[decode_out_n.rs2];
        }
    } else {
    // Interlock check
    // Check for hazards on rs1 and rs2
        if ((((decode_out_n.rs1 != 0) && re_reg1) && ((we_exe && (decode_out_n.rs1 == ws_exe)) || // Hazard with execute stage
                (we_mem && (decode_out_n.rs1 == ws_mem)) || // Hazard with memory stage
                (we_wb && (decode_out_n.rs1 == ws_wb)))) || // Hazard with writeback stage
                (((decode_out_n.rs2 != 0) && re_reg2) && ((we_exe && (decode_out_n.rs2 == ws_exe)) || // Hazard with execute stage
                (we_mem && (decode_out_n.rs2 == ws_mem)) || // Hazard with memory stage
                (we_wb && (decode_out_n.rs2 == ws_wb))))) { // Hazard with writeback stage
                pipe_stall = 1;
                return nop; // Stall pipeline with a NOP
        }
    }

    // determine the instruction type based on the opcode
    switch (decode_out_n.opcode) {
        case RTYPE: // R-Type Instructions - opcode = 0x33
            decode_out_n.alu_in1 = registers[decode_out_n.rs1];
            decode_out_n.alu_in2 = registers[decode_out_n.rs2];
            break;

        case ITYPE_LOAD:
        case ITYPE_ARITH:
        case STYPE:
            // case LUI: // I-Type Instructions - opcode = 0x13 or 0x3
            decode_out_n.alu_in1 = registers[decode_out_n.rs1];
            decode_out_n.alu_in2 = decode_out_n.imm;

            if (decode_out_n.opcode == STYPE) 
                decode_out_n.mem_buffer = registers[decode_out_n.rs2];
            
            break;

        case LUI:
            // decode_out_n.imm = decode_out_n.inst & bit_31_downto_12; // last 12 bits set to 0
            break;


        case JAL:
            j_taken = 1;
            decode_out_n.link_addr = decode_out_n.inst_addr + 4;
            pc_n = decode_out_n.inst_addr + decode_out_n.imm;
        break;
        case JALR:
            j_taken = 1;
            // alu to rs1 and alu to rs2
            decode_out_n.alu_in1 = registers[decode_out_n.rs1];
            decode_out_n.alu_in2 = decode_out_n.imm;
            decode_out_n.link_addr = decode_out_n.inst_addr + 4;
            pc_n = registers[decode_out_n.rs1] + decode_out_n.imm;
            break;

        case BTYPE:
            /* registers[rs1] and registers[rs2] are read as the two ALU operands. br_addr is
            set to inst_addr + immediate. */
            decode_out_n.alu_in1 = registers[decode_out_n.rs1];
            decode_out_n.alu_in2 = registers[decode_out_n.rs2];
            decode_out_n.br_addr = decode_out_n.inst_addr + decode_out_n.imm;
            break;

        default:
            break;
        }

  return decode_out_n;
}

/**
 * Execute stage implementation
 */
struct State execute() {

    we_exe = 0;
    ws_exe = 0;
    dout_exe = 0;
    br_mispredicted = 0;

    ex_out_n = decode_out;

    // Do a check if nop is passed and return the same
    // NOP or load/store handling
    if (decode_out.inst == nop.inst || decode_out.opcode == ITYPE_LOAD || decode_out.opcode == STYPE) {
        return nop;
    }



    // use Switch case for setting ws_exe and we_exe
    switch (ex_out_n.opcode) {
        case RTYPE:
            if (ex_out_n.funct3 == ADD_SUB) {
                if(ex_out_n.funct7 == SUB_F7){
                ex_out_n.alu_out = ex_out_n.alu_in1 - ex_out_n.alu_in2;
                } else {
                ex_out_n.alu_out = ex_out_n.alu_in1 + ex_out_n.alu_in2;
                }
            } else if (ex_out_n.funct3 == SLT) {
                ex_out_n.alu_out = (ex_out_n.alu_in1 < ex_out_n.alu_in2) ? 1 : 0;
            } else if (ex_out_n.funct3 == SLL) {
                ex_out_n.alu_out = ex_out_n.alu_in1 << ex_out_n.alu_in2;
            } else if (ex_out_n.funct3 == SRL) {
                ex_out_n.alu_out = ex_out_n.alu_in1 >> ex_out_n.alu_in2;
            } else if (ex_out_n.funct3 == AND) {
                ex_out_n.alu_out = ex_out_n.alu_in1 & ex_out_n.alu_in2;
            } else if (ex_out_n.funct3 == OR) {
                ex_out_n.alu_out = ex_out_n.alu_in1 | ex_out_n.alu_in2;
            } else if (ex_out_n.funct3 == XOR) {
                ex_out_n.alu_out = ex_out_n.alu_in1 ^ ex_out_n.alu_in2;
            } else {}
            we_exe = 1;
            ws_exe = ex_out_n.rd;
            dout_exe = ex_out_n.alu_out;

        break;


        case ITYPE_ARITH:
            // I-Type Instructions:  LW, ADDI, ANDI, ORI, XORI, SLTI, SLLI, SRLI
            if (ex_out_n.funct3 == ADD_SUB) { // check with TA here
                ex_out_n.alu_out = ex_out_n.alu_in1 + ex_out_n.imm;
            } else if (ex_out_n.funct3 == AND) { // ANDI
                ex_out_n.alu_out = ex_out_n.alu_in1 & ex_out_n.imm;
            } else if (ex_out_n.funct3 == OR) { // ORI
                ex_out_n.alu_out = ex_out_n.alu_in1 | ex_out_n.imm;
            } else if (ex_out_n.funct3 == XOR) { // XOR
                ex_out_n.alu_out = ex_out_n.alu_in1 ^ ex_out_n.imm; // check this again
            } else if (ex_out_n.funct3 == SLT) { // SLTI
                ex_out_n.alu_out = (ex_out_n.alu_in1 < ex_out_n.imm) ? 1 : 0;
            } else if (ex_out_n.funct3 == SLL) { // SLLI
                ex_out_n.alu_out = ex_out_n.alu_in1 << ex_out_n.imm;
            } else if (ex_out_n.funct3 == SRL) { // SRLI
                ex_out_n.alu_out = ex_out_n.alu_in1 >> ex_out_n.imm;
            } else {}

            we_exe = 1;
            ws_exe = ex_out_n.rd;
            dout_exe = ex_out_n.alu_out;
            break;

            case LUI:
            // The upper 20 bits of the ALU output is set to the immediate value. The lower 12 bits
            // of the ALU are set to 0
            ex_out_n.alu_out = ex_out_n.inst & bit_31_downto_12;
            we_exe = 1;
            ws_exe = ex_out_n.rd;
            dout_exe = ex_out_n.alu_out;

        break;

        case BTYPE:
            if ((ex_out_n.funct3 == BEQ && ex_out_n.alu_in1 == ex_out_n.alu_in2) ||
                (ex_out_n.funct3 == BNE && ex_out_n.alu_in1 != ex_out_n.alu_in2) ||
                (ex_out_n.funct3 == BLT && ex_out_n.alu_in1 <  ex_out_n.alu_in2) ||
                (ex_out_n.funct3 == BGE && ex_out_n.alu_in1 >= ex_out_n.alu_in2)) {
                    br_mispredicted = 1;
                    pc_n = ex_out_n.br_addr;
            } else {
                br_mispredicted = 0;
            }

            we_exe = 0;
        break;

        case JAL:
        case JALR:
            dout_exe = ex_out_n.link_addr; // ask the TA
            pc_n = ex_out_n.link_addr; // settng the pc to new address  
            we_exe = 1;
            ws_exe = ex_out_n.rd;
            break;


        default:
        break;
        }


    return ex_out_n;
}



/**
 * Execute stage implementation for Load and Stores
 */
struct State execute_ld_st() {

    // Handling non-load/store instructions
    if (decode_out.inst == nop.inst || !(decode_out.opcode == ITYPE_LOAD || decode_out.opcode == STYPE)) {
        dmem_busy = 0;
        return nop;
    }

    // Copy decode_out to ex_ld_st_out_n
    ex_ld_st_out_n = decode_out;
    

    // If dmem is 0, it must be the first cycle of the memory access
    if (dmem_busy == 0 && (ex_ld_st_out_n.opcode == ITYPE_LOAD || ex_ld_st_out_n.opcode == STYPE)) {
        dmem_busy = 1; // Indicate that the memory is busy
        dmem_cycles = 6;    // Set the number of cycles for memory access
        
          // Memory address calculation for both load and store instructions
        ex_ld_st_out_n.mem_addr = ex_ld_st_out_n.alu_in1 + ex_ld_st_out_n.imm; // Calculate the memory address
    }

    // Track the number of cycles for memory access for multi-cycle operations
    if(dmem_busy == 1) {
        dmem_cycles--;

        // for last cycle of memory access, perform the memory operation
        if(dmem_cycles == 0) {
            dmem_busy = 0; // memory Operation is complete here, so set dmem_busy to 0

            // Load Word
            if (ex_ld_st_out_n.opcode == ITYPE_LOAD) {
                ex_ld_st_out_n.mem_buffer = memory[ex_ld_st_out_n.mem_addr];

                // Update global variables for the memory stage
                we_mem = 1;  // Write enable for memory stage
                ws_mem = ex_ld_st_out_n.rd;  // Destination register for memory stage
                dout_mem = ex_ld_st_out_n.mem_buffer;  // Data to write back
            }
            
            // Store Word
            if (ex_ld_st_out_n.opcode == STYPE) {
                memory[ex_ld_st_out_n.mem_addr] = ex_ld_st_out_n.mem_buffer;
                we_mem = 0;  // No writeback for store operation
            }

            
        }

    }
    
    return ex_ld_st_out_n;  

}


/**
 * Writeback stage implementation
 */
unsigned int writeback() {

    dout_wb = 0;
    we_wb = 0;
    ws_wb = 0;

    // Case 1: Both ex_out and ex_ld_st_out are NOP, return NOP
    if (ex_out.inst == nop.inst && ex_ld_st_out.inst == nop.inst) {
        return nop.inst;  // No valid instruction to commit
    }

    // Case 2: ex_ld_st_out contains a valid load/store but dmem_busy == 1, stall the pipeline
    if (ex_ld_st_out.inst != nop.inst && dmem_busy == 1) {
        return nop.inst;  // Stall writeback stage as memory operation is not finished yet
    }

    // Case 3: ex_ld_st_out contains a valid load instruction, memory access is complete
    if (ex_ld_st_out.inst != nop.inst && (ex_ld_st_out.opcode == ITYPE_LOAD && dmem_busy == 0)) {
        // Populate wb_out_n based on the load operation
        wb_out_n = ex_ld_st_out;
        // wb_out_n.alu_out = ex_ld_st_out.mem_buffer;

        // Write the loaded value into the destination register
        registers[ex_ld_st_out.rd] = ex_ld_st_out.mem_buffer;

        // Update global variables for writeback
        we_wb = 1;
        ws_wb = ex_ld_st_out.rd;
        dout_wb = ex_ld_st_out.mem_buffer;

        return wb_out_n.inst;  // Return the committed load instruction
    }

    // Case 4: ex_out contains a valid ALU or JAL/JALR instruction, no memory operation
    if (ex_out.inst != nop.inst && ex_ld_st_out.inst == nop.inst) {
        // Populate wb_out_n based on the instruction
        wb_out_n = ex_out;

        // Writeback stage
        if (wb_out_n.opcode == RTYPE || wb_out_n.opcode == ITYPE_ARITH || wb_out_n.opcode == LUI) {
            registers[wb_out_n.rd] = wb_out_n.alu_out;
            dout_wb = wb_out_n.alu_out;
        }  else if (wb_out_n.opcode == JAL || wb_out_n.opcode == JALR) {
            if(wb_out_n.rd != 0){
                registers[wb_out_n.rd] = wb_out_n.link_addr;
            }
            dout_wb = wb_out_n.link_addr;
            // dout_wb = wb_out_n.alu_out;
            
        }

        // Update global variables for writeback
        we_wb = 1;
        ws_wb = ex_out.rd;

        return wb_out_n.inst;  // Return the committed instruction
    }

    return nop.inst;  // Default: no instruction committed
}


/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}


