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
    
    return fetch_out; 
}

/**
 * Execute stage implementation
 */
struct State execute(struct State decode_out) {

    /**
     * TODO: Your code for the decode stage here.
    */

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