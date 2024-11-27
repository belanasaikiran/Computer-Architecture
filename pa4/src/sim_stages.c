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
    
    /**
     * TODO: Your code for the decode stage here.
    */

}

/**
 * Decode stage implementation
 */
struct State decode() {
    
    /**
     * TODO: Your code for the decode stage here.
    */

}

/**
 * Execute stage implementation
 */
struct State execute() {
    
    /**
     * TODO: Your code for the decode stage here.
    */

}

/**
 * Execute stage implementation for Load and Stores
 */
struct State execute_ld_st() {
    
    /**
     * TODO: Your code for the decode stage here.
    */

}

/**
 * Execute stage implementation for second Load and Stores
 */
struct State execute_2nd_ld_st() {

    /**
     * TODO: Your code for the decode stage here.
    */

}

/**
 * Writeback stage implementation
 */
void writeback() {

    /**
     * TODO: Your code for the decode stage here.
    */
    
}



/**
 * Advance PC.
 * DO NOT MODIFY.
 */

void advance_pc(int step) {
    pc_n = step;
}

