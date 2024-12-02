/**
 * University of Connecticut
 * CSE 4302: Computer Architecture
 * Fall 2024
 * Hanan Khan (Reference Implementation)
 * 
 * Programming Assignment 4: OOO
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

    // Stall or Flush handling
    if(ooo_enabled) {
        if(pipe_stall) return fetch_out;
    } else {
        if (dmem_busy == 1 || pipe_stall == 1) return fetch_out; // stall fetch when memory (LD/SW) is busy,Return current state if stalled
    }

    // Handling control hazards: return nop if jump or branch is taken
    if (j_taken == 1 || br_mispredicted == 1) return nop; // inserting a bubble

    fetch_out_n = fetch_out; // Copy the curren t fetch state to the next state
    fetch_out_n.inst = memory[pc / 4]; // Fetch new instruction
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

    // Copy fetch_out to decodse_out_n
    decode_out_n = fetch_out;

   // Control hazard handling
    if (br_mispredicted == 1) return nop; //branch misprediction - insert a bubble
    // if (dmem_busy == 1 && ooo_enabled == 0) return nop; // stall decode when memory (LD/SW) is busy

    // reset
    pipe_stall = 0; // reset the pipe_stall flag
    j_taken = 0;
    br_mispredicted = 0;
    int re_reg1 = 0, re_reg2 = 0; // initiate the read enable registers
    int raw_hazard = 0; // initiate the raw hazard flag
    int waw_hazard = 0; // initiate the write after write hazard flag
    int war_hazard = 0; // initiate the write after read hazard flag
    decode_fields( &decode_out_n);

    // Interlock logic
    if (decode_out_n.opcode == RTYPE || decode_out_n.opcode == STYPE || decode_out_n.opcode == BTYPE) {
        re_reg1 = 1;
        re_reg2 = 1;
    } else if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == ITYPE_ARITH || decode_out_n.opcode == JALR) {
        re_reg1 = 1;
    }


    // *******************************************************************
    // ************ Out of order execution check and handling ************
    // *******************************************************************
    if(ooo_enabled){

        if(decode_out_n.rd == 0){
            if (dmem_busy || dmem_busy2) {
                pipe_stall = 1;
                return nop;
            }
        }

            // Checking for Terminate Instruction and handling
            /* For terminate instruction (addi zero,zero,1) */
            if(decode_out_n.opcode == ITYPE_ARITH && // opcode is ADDI
                decode_out_n.rd == 0 && // destination register is  0
                decode_out_n.rs1 == 0 && // source register is 0
                decode_out_n.imm == 1){ // immediate value is 1

                if (dmem_busy || dmem_busy2) {
                    pipe_stall = 1;
                    return nop;
                }
            }


        // checks for:
        // - RAW hazards
        // - WAW hazards
        // - WAR hazards
        if(decode_out_n.opcode != STYPE && decode_out_n.opcode != ITYPE_LOAD){
            // Check for WAW hazards
            if ((we_mem && (decode_out_n.rd == ws_mem)) ||
                (we_mem2 && (decode_out_n.rd == ws_mem2))) {
                waw_hazard = 1;
            }
        }

        // assigning load/store to available unit
        if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE) {

            //Checking for WAR hazards
            if ((we_mem && (registers[decode_out_n.rs1] + decode_out_n.imm == ex_ld_st_out.mem_addr)) || 
                (we_mem2 && (registers[decode_out_n.rs1] + decode_out_n.imm == ex_ld_st_2_out.mem_addr))) {
                war_hazard = 1;
            }


            if (dmem_busy == 0) decode_out_n.ld_st_unit = 1;
            else if (dmem_busy2 == 0) decode_out_n.ld_st_unit = 2;
            else {
                pipe_stall = 1;
                return nop; // Stall if both units are busy
            }
        }


        // RAW hazard detection
        if (re_reg1 && decode_out_n.rs1 != 0) {
            if (we_mem && decode_out_n.rs1 == ws_mem && dmem_busy) raw_hazard = 1;
            else if (we_mem2 && decode_out_n.rs1 == ws_mem2 && dmem_busy2) raw_hazard = 1;
        }
        if (re_reg2 && decode_out_n.rs2 != 0) {
            if (we_mem && decode_out_n.rs2 == ws_mem && dmem_busy) raw_hazard = 1;
            else if (we_mem2 && decode_out_n.rs2 == ws_mem2 && dmem_busy2) raw_hazard = 1;
        }


    } else { // In-order execution


        if(dmem_busy == 1){
            pipe_stall = 1;
            return nop;
        }


        if (decode_out_n.opcode == ITYPE_LOAD || decode_out_n.opcode == STYPE) {
            decode_out_n.ld_st_unit = 1; // Always use the first unit
            if (dmem_busy) {
                pipe_stall = 1;
                return nop;
            }
        }
    }



    // 4. Check for forwarding enabled - both ooo and in-order
    if (forwarding_enabled) {

        // stall the pipeline if there is a hazard
        if((decode_out_n.rs1 != 0) && re_reg1 == 1){
            if(we_exe && decode_out_n.rs1 == ws_exe) registers[decode_out_n.rs1] = dout_exe;
            else if(we_mem && decode_out_n.rs1 == ws_mem) registers[decode_out_n.rs1] = dout_mem;
            else if(we_wb && decode_out_n.rs1 == ws_wb) registers[decode_out_n.rs1] = dout_wb;

            // out of order checks
            if(ooo_enabled){
                if(we_ld_st_wb && decode_out_n.rs1 == ws_ld_st_wb) registers[decode_out_n.rs1] = dout_ld_st_wb;
                else if(we_ld_st_2_wb && decode_out_n.rs1 == ws_ld_st_2_wb) registers[decode_out_n.rs1] = dout_ld_st_2_wb;
                else if(we_mem2 && decode_out_n.rs1 == ws_mem2) registers[decode_out_n.rs1] = dout_mem2;
            }
            decode_out_n.alu_in1 = registers[decode_out_n.rs1];

        }

        if((decode_out_n.rs2 != 0) && re_reg2 == 1){
            if(we_exe && decode_out_n.rs2 == ws_exe) registers[decode_out_n.rs2] = dout_exe;
            else if(we_mem && decode_out_n.rs2 == ws_mem) registers[decode_out_n.rs2]  = dout_mem;
            else if(we_wb && decode_out_n.rs2 == ws_wb) registers[decode_out_n.rs2]  = dout_wb;

            // out of order checks
            if(ooo_enabled){
                if(we_ld_st_wb && decode_out_n.rs2 == ws_ld_st_wb) registers[decode_out_n.rs2] = dout_ld_st_wb;
                else if(we_ld_st_2_wb && decode_out_n.rs2 == ws_ld_st_2_wb) registers[decode_out_n.rs2] = dout_ld_st_2_wb;
                else if(we_mem2 && decode_out_n.rs2 == ws_mem2) registers[decode_out_n.rs2] = dout_mem2;
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
                raw_hazard = 1;
        }
    }

    // for both in-order and ooo
    if(raw_hazard == 1 || waw_hazard == 1 || war_hazard == 1){
        pipe_stall = 1;
        return nop;
    }


    // 5. Determine the instruction type based on the opcode
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
    ws_exe = decode_out.rd;
    br_mispredicted = 0;


    // Do a check if nop is passed and return the same
    // NOP or load/store handling
    if(ooo_enabled){
        if (decode_out.opcode == ITYPE_LOAD || decode_out.opcode == STYPE) return nop;
    } else{
        if (decode_out.opcode == ITYPE_LOAD || decode_out.opcode == STYPE || dmem_busy == 1) return nop;
    }


    ex_out_n = decode_out;

    // use Switch case for setting ws_exe and we_exe
    switch (ex_out_n.opcode) {
        case RTYPE:
        case ITYPE_ARITH:
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
            dout_exe = ex_out_n.alu_out;

        break;


        case LUI:
            we_exe = 1;
            ex_out_n.alu_out = ex_out_n.inst & bit_31_downto_12;
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

        break;

        case JAL:
        case JALR:
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

    we_mem = 0;
    ws_mem = 0;
    // dout_mem = 0;
    ex_ld_st_out_n = decode_out; // Copy decode_out to ex_ld_st_out_n

    if(dmem_busy == 0 || ex_ld_st_out_n.ld_st_unit != 1){
        return nop;
    }

    // for non-memory instructions, return nop
    if (dmem_busy == 0 && (ex_ld_st_out_n.opcode != ITYPE_LOAD || ex_ld_st_out_n.opcode != STYPE)) {
        return nop;
    }

    // If dmem is 0, it must be the first cycle of the memory access
    if (dmem_busy == 0 && (ex_ld_st_out_n.opcode == ITYPE_LOAD || ex_ld_st_out_n.opcode == STYPE)) {
        dmem_busy = 1; // Indicate that the memory is busy
        dmem_cycles = 1;    // Set the number of cycles for memory access
        ex_ld_st_out_n.mem_addr = ex_ld_st_out_n.alu_in1 + ex_ld_st_out_n.alu_in2; // Calculate the memory address
        return ex_ld_st_out_n;  // Return initialized state for first cycle
    }

    // Multi-cycle handling
    if (dmem_busy == 1) {
        ex_ld_st_out_n = ex_ld_st_out;
        dmem_cycles++; // Decrement cycle count

        // for last cycle of memory access, perform the memory operation
        if(dmem_cycles == dmem_access_cycles) {
            // Load or Store operation
            if (ex_ld_st_out_n.opcode == ITYPE_LOAD) {
                ex_ld_st_out_n.mem_buffer = memory[ex_ld_st_out_n.mem_addr]; // Load value
                we_mem = 1;                 // Enable write for memory stage
                if(ex_ld_st_out_n.rd != 0) ws_mem = ex_ld_st_out_n.rd;
            } else if (ex_ld_st_out_n.opcode == STYPE) {
                memory[ex_ld_st_out_n.mem_addr] = ex_ld_st_out_n.mem_buffer; // Store value
                ws_mem = 0;
                we_mem =0;
            }
            dout_mem = ex_ld_st_out_n.mem_buffer; // Data to write back
            dmem_busy=0;
            dmem_cycles=0;
        }
        return ex_ld_st_out_n;
    }

}

/**
 * Execute stage implementation for second Load and Stores
 */
struct State execute_2nd_ld_st() {

    we_mem2 = 0;
    ws_mem2 = 0;
    ex_ld_st_2_out_n = decode_out;


    if(dmem_busy2 == 0 && ex_ld_st_2_out_n.ld_st_unit != 2){ 
        return nop;
    }

    if (dmem_busy2 == 0 && (ex_ld_st_2_out_n.opcode == ITYPE_LOAD || ex_ld_st_2_out_n.opcode == STYPE)) {
        dmem_busy2 = 1;
        dmem_cycles2 = 1;
        ex_ld_st_2_out_n.mem_addr = ex_ld_st_2_out_n.alu_in1 + ex_ld_st_2_out_n.alu_in2;
        return ex_ld_st_2_out_n;
    } 

    if (dmem_busy2 == 1) {
        ex_ld_st_2_out_n = ex_ld_st_2_out;
        dmem_cycles2++;

        if (dmem_cycles2 == dmem_access_cycles) {
            if (ex_ld_st_2_out_n.opcode == ITYPE_LOAD) {
                ex_ld_st_2_out_n.mem_buffer = memory[ex_ld_st_2_out_n.mem_addr];
                we_mem2 = 1;
                ws_mem2 = ex_ld_st_2_out_n.rd;
            } else if (ex_ld_st_2_out_n.opcode == STYPE) {
                memory[ex_ld_st_2_out_n.mem_addr] = ex_ld_st_2_out_n.mem_buffer;
                ws_mem2 = 0;
                we_mem2 = 0;
            }
            dout_mem2 = ex_ld_st_2_out_n.mem_buffer;
            dmem_busy2 = 0;
            dmem_cycles2 = 0;
        }
        return ex_ld_st_2_out_n;
    }

    return nop;

}

/**
 * Writeback stage implementation
 */
void writeback() {

    we_wb = 0;
    we_ld_st_wb = 0;
    we_ld_st_2_wb = 0;

    // PA 4 logic

    if(ooo_enabled){ // out of order
        // write back for non-memory instructions
        if(ex_out.inst != nop.inst){
            wb_out_n = ex_out;
            if(ex_out.opcode == RTYPE || ex_out.opcode == ITYPE_ARITH || ex_out.opcode == LUI){
                registers[ex_out.rd] = ex_out.alu_out;
                we_wb = 1;
                ws_wb = ex_out.rd;
                dout_wb = ex_out.alu_out;
            } else if(ex_out.opcode == JAL || ex_out.opcode == JALR){
                if(ex_out.rd != 0){
                    registers[ex_out.rd] = ex_out.link_addr;
                }
                we_wb = 1;
                ws_wb = ex_out.rd;
                dout_wb = ex_out.link_addr;
            }
        } else {
            wb_out_n = nop;
        }


        // write back for memory instructions - first load/store unit
        if(ex_ld_st_out.inst != nop.inst && dmem_busy == 0){
            wb_ld_st_out_n = ex_ld_st_out;
            if(wb_out_n.opcode == ITYPE_LOAD){
                registers[wb_out_n.rd] = ex_ld_st_out.mem_buffer;
                we_ld_st_wb = 1;
                ws_ld_st_wb = wb_out_n.rd;
                dout_ld_st_wb = ex_ld_st_out.mem_buffer;
            }
        } else {
            wb_ld_st_out_n = nop;
        }

        // write back for memory instructions - second load/store unit
        if(ex_ld_st_2_out.inst != nop.inst && dmem_busy2 == 0){
            wb_ld_st_2_out_n = ex_ld_st_2_out;
            if(wb_out_n.opcode == ITYPE_LOAD){
                registers[wb_out_n.rd] = ex_ld_st_2_out.mem_buffer;
                we_ld_st_2_wb = 1;
                ws_ld_st_2_wb = wb_out_n.rd;
                dout_ld_st_2_wb = ex_ld_st_2_out.mem_buffer;
            }
        } else {
            wb_ld_st_2_out_n = nop;
        }

        // if(we_ld_st_wb && we_ld_st_2_wb && ws_ld_st_wb == ws_ld_st_2_wb){
        //     we_ld_st_2_wb = 0;
        //     ws_ld_st_2_wb = 0;
        //     dout_ld_st_2_wb = 0;
        // }


        // return instruction or nop
        // if (ex_out.inst != nop.inst || ex_ld_st_out.inst != nop.inst || ex_ld_st_2_out.inst != nop.inst)  return;

    } else {
        // in order writeback

        // if dmem_busy or dmem_busy2 is 1, return nop
        if(dmem_busy || ex_out.inst == nop.inst || ex_ld_st_out.inst == nop.inst) wb_out_n = nop;


        // ex_ld_st_out contains a valid load/store but dmem_busy == 1, stall the pipeline
        if (dmem_busy && (ex_ld_st_out.opcode == ITYPE_LOAD || ex_ld_st_out.opcode == STYPE)) wb_out_n = nop;  // Stall writeback stage as memory operation is not finished yet

        // priority order: execute -> load/store -> load/store 2
        if(ex_out.inst != nop.inst && dmem_busy == 0){
            wb_out_n = ex_out;
            if(ex_out.opcode == RTYPE || ex_out.opcode == ITYPE_ARITH || ex_out.opcode == LUI){
                registers[ex_out.rd] = ex_out.alu_out; // Writing the ALU result to the destination register
                we_wb = 1; // Enable writeback
                ws_wb = ex_out.rd; // Set the writeback register
                dout_wb = ex_out.alu_out; // forward the ALU result
            } else if(ex_out.opcode == JAL || ex_out.opcode == JALR){
                if(ex_out.rd != 0){
                    registers[ex_out.rd] = ex_out.link_addr; // Writing the link address to the destination register
                }
                we_wb = 1;
                ws_wb = ex_out.rd;
                dout_wb = ex_out.link_addr;
            }
        }

        //LOAD 
        if(ex_ld_st_out.inst != nop.inst && dmem_busy == 0){
            wb_out_n = ex_ld_st_out;
            if(wb_out_n.opcode == ITYPE_LOAD){
                registers[wb_out_n.rd] = ex_ld_st_out.mem_buffer; // Writing the loaded value into the destination register
                we_ld_st_wb = 1; // Enable writeback for load/store unit 1
                ws_ld_st_wb = wb_out_n.rd; // Set the writeback register
                dout_ld_st_wb = ex_ld_st_out.mem_buffer; // forward the loaded value
            }
        }
    }
}



/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}

