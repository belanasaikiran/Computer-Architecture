/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 * 
 * riscy-uconn: sim_stages.c
 * 
 *  MODIFY THIS FILE! 
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
int debug = 0;          // Set to 1 for additional debugging information.
int pipe_trace = 1;     // Set to 1 for pipe trace.
int pipe_trace_mode = 3;

/**
 * Fetch stage implementation.
 */
struct State fetch(struct State fetch_out) {
    
    //Use the current PC to store i) The instruction ii) The instruction address
    struct State fetch_out_temp = {0};
    fetch_out_temp.inst = memory[pc / 4];
    fetch_out_temp.inst_addr = pc;

    //Advance the (next) PC 
    advance_pc(fetch_out_temp.inst_addr + 4);
    
    //Return the instruction
    return fetch_out_temp;
}


/**
 * Decode stage implementation
 */
struct State decode(struct State fetch_out) {

    // Start decoding instructions //
    decode_fields(&fetch_out);

    fetch_out.alu_in1 = registers[fetch_out.rs1];
    fetch_out.alu_in2 = fetch_out.imm;
    
    return fetch_out;    

}

/**
 * Execute stage implementation
 */
struct State execute(struct State decode_out)
{
    decode_out.alu_out = decode_out.alu_in1 + decode_out.alu_in2;
    return decode_out;
}

/**
 * Memory stage implementation
 */
struct State memory_stage(struct State ex_out) {
    
    //dont do anything

    return ex_out;
}


/**
 * Writeback stage implementation
 */
unsigned int writeback(struct State mem_out) {

    registers[mem_out.rd] = mem_out.alu_out; //All these write ALU output to register

    return mem_out.inst;
}

/**
 * Advance PC.
 * DO NOT MODIFY.
 */
void advance_pc(int step) {
    pc_n = step;
}